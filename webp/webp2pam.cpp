#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>

using namespace std;

const bool DEBUG = true;

const uint32_t MAGICNUMBER = 0x46464952;  // RIFF (little-endian)
const uint32_t WEBP = 0x50424557;         // WEBP (little-endian)
const uint32_t VP8L = 0x4c385056;         // VP8L (little-endian)

// Order in which code lengths are momorized in the stream
static const int kCodeLengthCodeOrder[19] = {
    17, 18, 0, 1, 2, 3, 4, 5, 16, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

static const int kDistanceTable[120][2] = {
    {0, 1}, {1, 0}, {1, 1}, {-1, 1}, {0, 2}, {2, 0}, {1, 2},
    {-1, 2}, {2, 1}, {-2, 1}, {2, 2}, {-2, 2}, {0, 3}, {3, 0},
    // ... all 120 entries from the docs ...
};

class HuffNode
{
public:
    int symbol = -1;
    HuffNode* left = nullptr;
    HuffNode* right = nullptr;

    HuffNode() : symbol(-1), left(nullptr), right(nullptr) {}
};

HuffNode* BuildHuffman(const vector<int>& lengths, int num_symbols)
{
    int max_len = 0;
    for (int i = 0; i < num_symbols; ++i)
        if (lengths[i] > max_len) max_len = lengths[i];

    vector<int> bl_count(max_len + 1, 0);
    for (int i = 0; i < num_symbols; ++i)
        if (lengths[i] > 0) ++bl_count[lengths[i]];

    vector<int> next_code(max_len + 1, 0);
    int code = 0;
    for (int bits = 1; bits <= max_len; ++bits) {
        code = (code + bl_count[bits - 1]) << 1;
        next_code[bits] = code;
    }

    HuffNode* root = new HuffNode();

    int nonzero = 0, only = -1;
    for (int i = 0; i < num_symbols; ++i)
        if (lengths[i] > 0) { ++nonzero; only = i; }
    if (nonzero == 1) { root->symbol = only; return root; }

    for (int sym = 0; sym < num_symbols; ++sym) {
        int len = lengths[sym];
        if (len == 0) continue;
        int c = next_code[len]++;
        HuffNode* node = root;
        for (int i = len - 1; i >= 0; --i) {
            int bit = (c >> i) & 1;
            HuffNode*& child = bit ? node->right : node->left;
            if (!child) child = new HuffNode();
            node = child;
        }
        node->symbol = sym;
    }
    return root;
}

class webp2pam
{
private:
    ifstream& is_;
    ofstream& os_;
    uint64_t buffer_ = 0;
    uint32_t chunk_len_;
    uint32_t lossless_stream_len_;
    uint32_t image_width_;
    uint32_t image_height_;
    size_t bits_ = 0;

    int alphabet_sizes[5] = {280, 256, 256, 256, 40};
    HuffNode* codes[5];

public:
    webp2pam(ifstream& is, ofstream& os) : is_(is), os_(os) {}
    ~webp2pam() = default;

    uint32_t read_bits(size_t size = 1)
    {
        while (bits_ < size) {
            buffer_ |= (uint64_t)(is_.get()) << bits_;
            bits_ += 8;
        }
        uint32_t val = buffer_ & ((1ull << size) - 1);
        buffer_ >>= size;
        bits_ -= size;
        return val;
    }

    bool read_magicnumber()
    {
        return read_bits(32) == MAGICNUMBER;
    }

    bool read_chunk_len()
    {
        chunk_len_ = read_bits(32);
        return true;
    }

    bool read_webp()
    {
        return read_bits(32) == WEBP;
    }

    bool read_vp8l()
    {
        return read_bits(32) == VP8L;
    }

    bool read_lossless_stream_len()
    {
        lossless_stream_len_ = read_bits(32);
        return true;
    }

    bool read_signature()
    {
        return read_bits(8) == 0x2f;
    }

    bool read_image_width()
    {
        image_width_ = read_bits(14) + 1;
        return true;
    }

    bool read_image_height()
    {
        image_height_ = read_bits(14) + 1;
        return true;
    }

    bool read_alpha()
    {
        return read_bits(1) == 0;   // alpha is set to 255 for every pixel
    }

    bool read_version()
    {
        return read_bits(3) == 0;   // version is 0
    }

    bool read_header()
    {
        return
            read_magicnumber() &&
            read_chunk_len() &&
            read_webp() &&
            read_vp8l() &&
            read_lossless_stream_len() &&
            read_signature() &&
            read_image_width() &&
            read_image_height() &&
            read_alpha() &&
            read_version();
    }

    // Writes the PAM header to the output file
    void write_pam_header()
    {
        os_ << "P7\n";
        os_ << "WIDTH " << image_width_ << "\n";
        os_ << "HEIGHT " << image_height_ << "\n";
        os_ << "DEPTH 4\n";
        os_ << "MAXVAL 255\n";
        os_ << "TUPLETYPE RGBA\n";
        os_ << "ENDHDR\n";
    }

    int huff_decode(HuffNode* root)
    {
        HuffNode* node = root;
        while (node && node->symbol < 0) {
            int bit = read_bits();
            node = bit ? node->right : node->left;
        }
        if (!node) throw std::runtime_error("huff_decode: fell off tree");
        return node->symbol;
    }

    HuffNode* ReadPrefixCode(int idx) {
        std::vector<int> lengths(alphabet_sizes[idx], 0);

        if (read_bits(1) == 1) {
            // Simple branch
            int num_symbols = read_bits(1) + 1;
            int is_first_8bits = read_bits(1);
            int symbol0 = read_bits(1 + 7 * is_first_8bits);
            if (num_symbols == 1) {
                // Single-symbol tree: always decodes to symbol0, no bits read
                HuffNode* r = new HuffNode();
                r->symbol = symbol0;
                return r;
            }
            lengths[symbol0] = 1;
            int symbol1 = read_bits(8);
            lengths[symbol1] = 1;
        } else {
            // Normal branch
        
            // Step 1: read the 19 inner lengths (LOCAL array)
            int num_code_lengths = 4 + read_bits(4);
            std::vector<int> inner_lengths(19, 0);
            for (int i = 0; i < num_code_lengths; ++i) {
                inner_lengths[kCodeLengthCodeOrder[i]] = read_bits(3);
            }
        
            // Step 2: build the inner tree (alphabet is always 19)
            HuffNode* inner_tree = BuildHuffman(inner_lengths, 19);
        
            // Step 3: read the max_symbol limiter
            int max_symbol = alphabet_sizes[idx];
            if (read_bits(1) == 1) {
                int length_nbits = 2 + 2 * read_bits(3);
                max_symbol = 2 + read_bits(length_nbits);
            }
        
            // Step 4: decode outer lengths through the inner tree, with RLE
            int prev_non_zero = 8;
            int symbol = 0;
            int read = 0;
            while (symbol < alphabet_sizes[idx] && read < max_symbol) {
                int s = huff_decode(inner_tree);    // uses your bit reader internally
                ++read;
                if (s < 16) {
                    lengths[symbol++] = s;
                    if (s != 0) prev_non_zero = s;
                } else if (s == 16) {
                    int repeat = 3 + read_bits(2);
                    while (repeat--) lengths[symbol++] = prev_non_zero;
                } else if (s == 17) {
                    int repeat = 3 + read_bits(3);
                    while (repeat--) lengths[symbol++] = 0;
                } else {  // s == 18
                    int repeat = 11 + read_bits(7);
                    while (repeat--) lengths[symbol++] = 0;
                }
            }
        }
    
        // Build the data tree from the now-complete length vector
        return BuildHuffman(lengths, alphabet_sizes[idx]);
    }

    int GetValueFromPrefix(int prefix_code) {
        if (prefix_code < 4) return prefix_code + 1;
        int extra_bits = (prefix_code - 2) >> 1;
        int offset = (2 + (prefix_code & 1)) << extra_bits;
        return offset + read_bits(extra_bits) + 1;
    }

    int MapDistance(int dist_code, int image_width) {
        if (dist_code > 120) return dist_code - 120;
        int xi = kDistanceTable[dist_code - 1][0];
        int yi = kDistanceTable[dist_code - 1][1];
        int dist = xi + yi * image_width;
        return dist < 1 ? 1 : dist;
    }

    // Main decoding function
    int decode() {
        if (!read_header()) return 4;

        if (DEBUG) {
            cerr << "Image size: " << image_width_ << " x " << image_height_ << endl;
        }

        read_bits(1);  // no transforms
        read_bits(1);  // no color cache
        read_bits(1);  // no meta prefix

        for (int i = 0; i < 5; ++i) {
            codes[i] = ReadPrefixCode(i);
        }

        // In-memory pixel buffer (ARGB as separate channels or packed uint32)
        size_t total_pixels = image_width_ * image_height_;
        std::vector<uint8_t> pixels(total_pixels * 4);  // RGBA bytes

        size_t pos = 0;
        while (pos < total_pixels) {
            int S = huff_decode(codes[0]);

            if (S < 256) {
                // Literal pixel: green is S, then read R, B, A
                int g = S;
                int r = huff_decode(codes[1]);
                int b = huff_decode(codes[2]);
                int a = huff_decode(codes[3]);
                pixels[pos * 4 + 0] = r;
                pixels[pos * 4 + 1] = g;
                pixels[pos * 4 + 2] = b;
                pixels[pos * 4 + 3] = a;
                ++pos;
            } else {
                // LZ77 backward reference
                int length_prefix = S - 256;
                int length = GetValueFromPrefix(length_prefix);

                int dist_prefix = huff_decode(codes[4]);
                int dist_code = GetValueFromPrefix(dist_prefix);

                int distance = MapDistance(dist_code, image_width_);
                if (distance < 1) distance = 1;

                for (int i = 0; i < length; ++i) {
                    // Copy one pixel (4 bytes) from pos - distance to pos
                    for (int c = 0; c < 4; ++c) {
                        pixels[pos * 4 + c] = pixels[(pos - distance) * 4 + c];
                    }
                    ++pos;
                }
            }
        }

        // Write header FIRST
        write_pam_header();

        // Then write pixel data
        os_.write(reinterpret_cast<const char*>(pixels.data()), pixels.size());

        return 0;
    }
};

int main(int argc, char* argv[])
{
    if (DEBUG) {
        char* debug_argv[] = {
            (char*)"./webp2pam",
            (char*)"/home/surface/Downloads/frog_notrans_1tile.webp",
            (char*)"/home/surface/Downloads/frog_from_webp.pam",
            nullptr
        };
        int debug_argc = 3;

        argv = debug_argv;
        argc = debug_argc;
    }

    if (argc != 3)
        exit(1);

    ifstream is(argv[1], ios::binary);
    if (!is)
        exit(2);

    ofstream os(argv[2], ios::binary);
    if (!os)
        exit(3);

    webp2pam w2p(is, os);

    return w2p.decode();
}
