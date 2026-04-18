#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

const bool DEBUG = true;

const uint32_t MAGICNUMBER = 0x46464952;  // RIFF (little-endian)
const uint32_t WEBP = 0x50424557;         // WEBP (little-endian)
const uint32_t VP8L = 0x4c385056;         // VP8L (little-endian)

// Order in which code lengths are momorized in the stream
static const int kCodeLengthCodeOrder[19] = {
    17, 18, 0, 1, 2, 3, 4, 5, 16, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

// Tables for length codes (G >= 256): offset and extra bits
static const int kLengthOffset[24] = {
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
    257, 385, 513, 769, 1025, 1537, 2049, 3073
};
static const int kLengthExtraBits[24] = {
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
    7, 7, 8, 8, 9, 9, 10, 10
};

// Tables for distance codes: offset and extra bits
static const int kDistanceOffset[40] = {
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
    257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193,
    12289, 16385, 24577, 32769, 49153, 65537, 98305, 131073, 196609,
    262145, 393217, 524289, 786433
};
static const int kDistanceExtraBits[40] = {
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
    7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13,
    14, 14, 15, 15, 16, 16, 17, 17, 18, 18
};

// 2D map for the first 120 distance codes (dx, dy) 
static const int8_t kDistanceMap[120][2] = {
    {0,1},{1,0},{1,1},{-1,1},{0,2},{2,0},{1,2},{-1,2},
    {2,1},{-2,1},{2,2},{-2,2},{0,3},{3,0},{1,3},{-1,3},
    {3,1},{-3,1},{2,3},{-2,3},{3,2},{-3,2},{0,4},{4,0},
    {1,4},{-1,4},{4,1},{-4,1},{3,3},{-3,3},{2,4},{-2,4},
    {4,2},{-4,2},{0,5},{3,4},{-3,4},{4,3},{-4,3},{5,0},
    {1,5},{-1,5},{5,1},{-5,1},{2,5},{-2,5},{5,2},{-5,2},
    {4,4},{-4,4},{3,5},{-3,5},{5,3},{-5,3},{0,6},{6,0},
    {1,6},{-1,6},{6,1},{-6,1},{2,6},{-2,6},{6,2},{-6,2},
    {4,5},{-4,5},{5,4},{-5,4},{3,6},{-3,6},{6,3},{-6,3},
    {0,7},{7,0},{1,7},{-1,7},{5,5},{-5,5},{7,1},{-7,1},
    {4,6},{-4,6},{6,4},{-6,4},{2,7},{-2,7},{7,2},{-7,2},
    {3,7},{-3,7},{7,3},{-7,3},{5,6},{-5,6},{6,5},{-6,5},
    {8,0},{4,7},{-4,7},{7,4},{-7,4},{8,1},{8,2},{6,6},
    {-6,6},{8,3},{5,7},{-5,7},{7,5},{-7,5},{8,4},{6,7},
    {-6,7},{7,6},{-7,6},{8,5},{7,7},{-7,7},{8,6},{8,7}
};

// ─── HuffmanTree ────────────────────────────────────────────────────────────────

class HuffmanTree
{
    struct Node {
        int symbol;  // >= 0 for leaf, -1 for internal node
        int left;    // index of left child for bit 0
        int right;   // index of right child for bit 1
        Node() : symbol(-1), left(-1), right(-1) {}
    };
    vector<Node> nodes_;

public:
    HuffmanTree() { nodes_.emplace_back(); } // create root node

    bool is_leaf(int idx) const { return nodes_[idx].symbol >= 0; }
    int  symbol(int idx)  const { return nodes_[idx].symbol; }
    int  left(int idx)    const { return nodes_[idx].left; }
    int  right(int idx)   const { return nodes_[idx].right; }
    bool empty()          const { return nodes_.size() <= 1 && !is_leaf(0); }

    // Builds the canonical Huffman tree from the code lengths.
    // code_lengths[i] = number of bits for symbol i (0 = not used).
    // Codes are inserted LSB-first (as required by WebP lossless).
    void build(const vector<int>& code_lengths)
    {
        nodes_.clear();
        nodes_.emplace_back(); // root

        int max_len = *max_element(code_lengths.begin(), code_lengths.end());
        if (max_len == 0) {
            // Special case: single symbol with length 0 (single-symbol tree)
            for (int i = 0; i < (int)code_lengths.size(); i++) {
                if (code_lengths[i] == 0) {
                    // Could be the only symbol used
                }
            }
            return;
        }

        // 1. Count how many codes there are for each length
        vector<int> bl_count(max_len + 1, 0);
        for (int len : code_lengths)
            if (len > 0) bl_count[len]++;

        // 2. Calculate the initial code for each length (canonical Huffman)
        vector<int> next_code(max_len + 1, 0);
        int code = 0;
        for (int bits = 1; bits <= max_len; bits++) {
            code = (code + bl_count[bits - 1]) << 1;
            next_code[bits] = code;
        }

        // 3. Insert each symbol into the tree (LSB-first for WebP)
        for (int sym = 0; sym < (int)code_lengths.size(); sym++) {
            int len = code_lengths[sym];
            if (len == 0) continue;
            int c = next_code[len]++;

            // Create the path in the tree MSB-first (from most significant bit to least).
            // This works because the WebP encoder reverses canonical codes before
            // writing them into the LSB-first bitstream, so read_bits(1) returns
            // the MSB of the canonical code first.
            int cur = 0;
            for (int bit = len - 1; bit >= 0; bit--) {
                int b = (c >> bit) & 1;
                if (b == 0) {
                    if (nodes_[cur].left < 0) {
                        nodes_[cur].left = (int)nodes_.size();
                        nodes_.emplace_back();
                    }
                    cur = nodes_[cur].left;
                } else {
                    if (nodes_[cur].right < 0) {
                        nodes_[cur].right = (int)nodes_.size();
                        nodes_.emplace_back();
                    }
                    cur = nodes_[cur].right;
                }
            }
            nodes_[cur].symbol = sym;
        }
    }
};

// helper function to build a HuffmanTree from a vector of code_lengths
HuffmanTree build_tree(const vector<int>& lengths, int /*alphabet_size*/)
{
    HuffmanTree tree;
    tree.build(lengths);
    return tree;
}

// ─── webp2pam ───────────────────────────────────────────────────────────────────

class webp2pam
{
private:
    ifstream& is_;
    ofstream& os_;
    uint64_t buffer_ = 0;
    uint32_t chuck_len_;
    uint32_t lossless_stream_len_;
    uint32_t image_width_;
    uint32_t image_height_;
    size_t bits_ = 0;

    HuffmanTree tree_G;
    HuffmanTree tree_R;
    HuffmanTree tree_B;
    HuffmanTree tree_A;
    HuffmanTree tree_dist;

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

    bool read_chuck_len()
    {
        chuck_len_ = read_bits(32);
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
            read_chuck_len() &&
            read_webp() &&
            read_vp8l() &&
            read_lossless_stream_len() &&
            read_signature() &&
            read_image_width() &&
            read_image_height() &&
            read_alpha() &&
            read_version();
    }

    // Decodes a symbol from the Huffman tree by reading bits from the stream
    int decode_symbol(const HuffmanTree& tree)
    {
        int node = 0; // radice
        while (!tree.is_leaf(node)) {
            int bit = read_bits(1);
            int next = bit ? tree.right(node) : tree.left(node);
            if (next < 0) {
                cerr << "ERROR: dead end in tree at node " << node
                     << " bit=" << bit << endl;
                return -1;
            }
            node = next;
        }
        return tree.symbol(node);
    }

    // Builds the code-of-codes tree and then decodes
    // the lengths for an alphabet of size alphabet_size.
    // Returns the vector of code_lengths.
    vector<int> read_normal_code_length(int alphabet_size)
    {
        // Level 1: code-of-codes
        int num_code_lengths = 4 + read_bits(4);
        if (DEBUG) cerr << "  num_code_lengths=" << num_code_lengths << " for alphabet=" << alphabet_size << endl;
        vector<int> code_length_code_lengths(19, 0);
        for (int i = 0; i < num_code_lengths; ++i)
            code_length_code_lengths[kCodeLengthCodeOrder[i]] = read_bits(3);

        if (DEBUG) {
            cerr << "  code_length_code_lengths: ";
            for (int j = 0; j < 19; j++) cerr << code_length_code_lengths[j] << " ";
            cerr << endl;
        }

        HuffmanTree code_of_codes = build_tree(code_length_code_lengths, 19);

        // Level 2: max_symbol
        int max_symbol;
        if (read_bits(1) == 0) {
            max_symbol = alphabet_size;
        } else {
            int length_nbits = 2 + 2 * read_bits(3);
            max_symbol = 2 + read_bits(length_nbits);
        }

        // Level 3: decode actual code lengths
        vector<int> code_lengths(alphabet_size, 0);
        int prev_nonzero = 8;
        int i = 0;
        while (i < max_symbol) {
            int code = decode_symbol(code_of_codes);
            if (code < 16) {
                code_lengths[i++] = code;
                if (code != 0) prev_nonzero = code;
            } else if (code == 16) {
                int repeat = 3 + read_bits(2);
                while (repeat-- && i < alphabet_size) code_lengths[i++] = prev_nonzero;
            } else if (code == 17) {
                int repeat = 3 + read_bits(3);
                while (repeat-- && i < alphabet_size) code_lengths[i++] = 0;
            } else { // 18
                int repeat = 11 + read_bits(7);
                while (repeat-- && i < alphabet_size) code_lengths[i++] = 0;
            }
        }
        return code_lengths;
    }

    // reads the Simple code and returns the vector of code_lengths
    vector<int> read_simple_code_length(int alphabet_size)
    {
        vector<int> code_lengths(alphabet_size, 0);
        int num_symbols = read_bits(1) + 1;
        int is_first_8bits = read_bits(1);
        int symbol0 = read_bits(1 + 7 * is_first_8bits);
        code_lengths[symbol0] = 1;
        if (num_symbols == 2) {
            int symbol1 = read_bits(8);
            code_lengths[symbol1] = 1;
        }
        return code_lengths;
    }

    // reads the 5 trees for the unique group of prefix codes
    void read_prefix_codes()
    {
        // G: alphabet 256 + 24 + color_cache_size (= 0)
        bool is_simple = read_bits(1);
        if (DEBUG) cerr << "Tree G: " << (is_simple ? "Simple" : "Normal") << endl;
        auto lengths_G = is_simple ? read_simple_code_length(280)
                                   : read_normal_code_length(280);

        is_simple = read_bits(1);
        if (DEBUG) cerr << "Tree R: " << (is_simple ? "Simple" : "Normal") << endl;
        auto lengths_R = is_simple ? read_simple_code_length(256)
                                   : read_normal_code_length(256);

        is_simple = read_bits(1);
        if (DEBUG) cerr << "Tree B: " << (is_simple ? "Simple" : "Normal") << endl;
        auto lengths_B = is_simple ? read_simple_code_length(256)
                                   : read_normal_code_length(256);

        is_simple = read_bits(1);
        if (DEBUG) cerr << "Tree A: " << (is_simple ? "Simple" : "Normal") << endl;
        auto lengths_A = is_simple ? read_simple_code_length(256)
                                   : read_normal_code_length(256);

        is_simple = read_bits(1);
        if (DEBUG) cerr << "Tree dist: " << (is_simple ? "Simple" : "Normal") << endl;
        auto lengths_dist = is_simple ? read_simple_code_length(40)
                                      : read_normal_code_length(40);

        // build the 5 final trees
        tree_G    = build_tree(lengths_G,    280);
        tree_R    = build_tree(lengths_R,    256);
        tree_B    = build_tree(lengths_B,    256);
        tree_A    = build_tree(lengths_A,    256);
        tree_dist = build_tree(lengths_dist, 40);
    }

    // Converts a distance code to the actual distance using the 2D map
    int distance_map(int dist_code)
    {
        if (dist_code > 120) {
            return dist_code - 120;
        }
        int dx = kDistanceMap[dist_code - 1][0];
        int dy = kDistanceMap[dist_code - 1][1];
        int dist = dy * (int)image_width_ + dx;
        return (dist < 1) ? 1 : dist;
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

    // Main decoding function
    int decode()
    {
        if (!read_header()) return 4;

        if (DEBUG) {
            cerr << "Image size: " << image_width_ << " x " << image_height_ << endl;
        }

        read_bits(1); // no transforms (0)
        read_bits(1); // no color cache (0)
        read_bits(1); // no meta prefix codes (0) -> un solo gruppo

        read_prefix_codes();

        int num_pixels = image_width_ * image_height_;
        // Buffer ARGB: 4 bytes per pixel (R, G, B, A)
        vector<uint8_t> pixels(num_pixels * 4);
        int pixel_idx = 0;

        while (pixel_idx < num_pixels) {
            int green = decode_symbol(tree_G);

            if (green < 256) {
                int red   = decode_symbol(tree_R);
                int blue  = decode_symbol(tree_B);
                int alpha = decode_symbol(tree_A);

                pixels[pixel_idx * 4 + 0] = (uint8_t)red;
                pixels[pixel_idx * 4 + 1] = (uint8_t)green;
                pixels[pixel_idx * 4 + 2] = (uint8_t)blue;
                pixels[pixel_idx * 4 + 3] = (uint8_t)alpha;
                pixel_idx++;
            } else if (green < 280) {
                int length_code = green - 256;
                int length = kLengthOffset[length_code];
                if (kLengthExtraBits[length_code] > 0)
                    length += read_bits(kLengthExtraBits[length_code]);

                int dist_code_raw = decode_symbol(tree_dist);
                int dist = kDistanceOffset[dist_code_raw];
                if (kDistanceExtraBits[dist_code_raw] > 0)
                    dist += read_bits(kDistanceExtraBits[dist_code_raw]);

                // Converts the 2D map for the distance
                int mapped_dist = distance_map(dist);

                // Copies 'length' pixels from the previous position
                for (int j = 0; j < length && pixel_idx < num_pixels; j++) {
                    int src = (pixel_idx - mapped_dist) * 4;
                    if (src < 0) src = 0;
                    pixels[pixel_idx * 4 + 0] = pixels[src + 0];
                    pixels[pixel_idx * 4 + 1] = pixels[src + 1];
                    pixels[pixel_idx * 4 + 2] = pixels[src + 2];
                    pixels[pixel_idx * 4 + 3] = pixels[src + 3];
                    pixel_idx++;
                }
            }
        }

        write_pam_header();
        os_.write(reinterpret_cast<const char*>(pixels.data()), pixels.size());

        if (DEBUG) {
            cerr << "Decoded " << pixel_idx << " pixels." << endl;
        }

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
