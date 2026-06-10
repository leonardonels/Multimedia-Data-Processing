// Same as huffman1.cpp (Classical)

#include <stdexcept>
#include <fstream>
#include <string>
#include <unordered_map>
#include <queue>
#include <iostream>

using namespace std;

const string MAGICNUMBER = "HUFFMAN3";

class BitReader {
    istream& is_;
    uint64_t buffer_ = 0;
    size_t bits_ = 0;

public:
    BitReader(istream& is) : is_(is) {}
    
    uint32_t read_bits(size_t size = 1)
    {
        while (bits_<size){
            buffer_ |= (uint64_t)(is_.get()) << bits_;
            bits_ += 8;
        }
        uint32_t val = buffer_ & ((1ull << size) - 1);
        buffer_ >>= size;
        bits_ -= size;
        return val;
    }
};

class BitWriter {
    ostream& os_;
    uint64_t buffer_ = 0;
    size_t bits_ = 0;

public:
    BitWriter(ostream& os) : os_(os) {}

    void write_bits(uint32_t val, size_t size = 1)
    {
        buffer_ |= (uint64_t)(val & ((1ull << size) - 1)) << bits_;
        bits_ += size;

        while (bits_ >= 8) {
            os_.put((char)(buffer_ & 0xFF));
            buffer_ >>= 8;
            bits_ -= 8;
        }
    }

    void flush()
    {
        if (bits_ > 0) {
            os_.put((char)(buffer_ & 0xFF));
            buffer_ = 0;
            bits_ = 0;
        }
    }

    ~BitWriter() { flush(); }
};

class huffman
{
private:
    struct node
    {
        uint8_t symbol_;
        uint32_t frequency_;
        node* left_;
        node* right_;
        node(uint8_t symbol, uint32_t frequency) : symbol_(symbol), frequency_(frequency), left_(nullptr), right_(nullptr){}
        node(node* left, node* right) : symbol_(0), frequency_(left->frequency_ + (right ? right->frequency_ : 0)), left_(left), right_(right){}
        ~node(){delete left_; delete right_; }
    };

    struct freq_cmp
    {
        bool operator()(const node* a, const node* b) const{
            return a->frequency_>b->frequency_;
        }
    };
    
    
public:
    huffman() = default;
    ~huffman() = default;

    void freq_count(ifstream& is, unordered_map<uint8_t, uint32_t>& frequency)
    {
        char c;
        while(is.get(c)){
            frequency[c]++;
        }
        is.clear();
        is.seekg(0, is.beg);
    }

    node* build_tree(unordered_map<uint8_t, uint32_t>& frequency)
    {
        priority_queue<node*, vector<node*>, freq_cmp> pq;

        for(const auto& [c, f] : frequency ){
            pq.push(new node(c, f));
        }

        if(pq.size()==1) return new node(pq.top(), nullptr);

        while(pq.size()>1){
            node* a = pq.top(); pq.pop();
            node* b = pq.top(); pq.pop();
            pq.push(new node(a, b));
        }

        return pq.top();
    }

    void build_codes(const node* n, const string& prefix, unordered_map<uint8_t, string>& codes) const
    {
        if(!n) return;
        if(!n->left_ && !n->right_){
            codes[n->symbol_] = prefix.empty() ? "0" : prefix;
            return;
        }
        build_codes(n->left_, prefix + '0', codes);
        build_codes(n->right_, prefix + '1', codes);
    }

    int encode(ifstream& is, ofstream& os)
    {
        unordered_map<uint8_t, uint32_t> frequency;
        freq_count(is, frequency);

        node* root = build_tree(frequency);

        unordered_map<uint8_t, string> codes;
        build_codes(root, "", codes);

        size_t max_len = 0;
        for (const auto& [s, cs] : codes)
            if (cs.size() > max_len) max_len = cs.size();
        cout << "Max code length: " << max_len << " (table entries: " << frequency.size() << ")" << endl;

        // [MAGICNUMBER]
        BitWriter bw(os);
        for(const auto& c : MAGICNUMBER){
            bw.write_bits(c, 8);
        }

        // [TABLE ENTRIES]
        bw.write_bits(frequency.size()!=256 ? frequency.size() : 0, 8);

        // [TABLE]
        for(auto [c, f] : frequency){
            const std::string& code_str = codes[c];
            uint32_t code_val = std::stoul(code_str, nullptr, 2);
            uint8_t code_len = static_cast<uint8_t>(code_str.size());
            bw.write_bits(c, 8);
            bw.write_bits(code_len, 5);
            bw.write_bits(code_val, code_len);
        }

        // [NUM SYMBOLS]
        uint32_t n=0;
        for(auto[_,f]:frequency)
            n+=f;
        bw.write_bits((n >> 24) & 0xFF, 8);
        bw.write_bits((n >> 16) & 0xFF, 8);
        bw.write_bits((n >>  8) & 0xFF, 8);
        bw.write_bits( n        & 0xFF, 8);
        cout << "Total symbols: " << n << endl;

        // [data]
        uint32_t n_symbols = 0;
        char c;
        while(is.get(c)){
            const std::string& code_str = codes[c];
            for(char c : code_str){
                bw.write_bits(c,1);
            }
            n_symbols++;
        }
        cout << "Encoded symbols: " << n_symbols << endl;

        return 0;
    }

    uint64_t make_key(uint8_t len, uint64_t code) {
        return ((uint64_t)len << 32) | code;
    }

    unordered_map<uint64_t, uint8_t> read_table(BitReader& br)
    {
        // [TABLE ENTRIES]
        uint16_t entries = br.read_bits(8);
        if(entries==0)
        entries=256;
        cout << "Table entries: " << entries << endl;
        
        // [TABLE]
        unordered_map<uint64_t, uint8_t> codes;
        for (size_t i = 0; i<entries; i++){
            uint8_t sym = br.read_bits(8);
            uint8_t len = br.read_bits(5);
            uint64_t code = br.read_bits(len);
            codes[make_key(len, code)] = sym;
        }

        return codes;
    }

    int decode(ifstream& is, ofstream& os)\
    {
        // [MAGICNUMBER]
        BitReader br(is);
        for (char expected : MAGICNUMBER) {
            uint8_t got = br.read_bits(8);
            if (got != (uint8_t)expected) exit(5);
        }
        
        // [TABLE ENTRIES]
        // [TABLE]
        unordered_map<uint64_t, uint8_t> codes = read_table(br);

        // [NUM SYMBOLS]
        uint32_t n = 0;
        n |= br.read_bits(8) << 24;
        n |= br.read_bits(8) << 16;
        n |= br.read_bits(8) <<  8;
        n |= br.read_bits(8);
        cout << "Total symbols to decode: " << n << endl;

        // [DATA]
        BitWriter bw(os);
        uint64_t c = 0;
        uint8_t len = 0;
        while(n>0){
            c = (c<<1) | br.read_bits();
            len += 1;
            if (codes.find(make_key(len, c)) != codes.end()){
                bw.write_bits(codes[make_key(len, c)], 8);
                c=0;
                len=0;
                n-=1;
            }
        }

        return 0;
    }
};

int main(int argc, char* argv[])
{
#ifdef DEBUG
    char* debug_argv[] = {
        (char*)".huffman3",
        (char*)"d",
        (char*)"/home/surface/Downloads/bibbia.huf",
        (char*)"/home/surface/Downloads/bibbia_d.txt",
        nullptr
    };
    int debug_argc = 4;

    argv = debug_argv;
    argc = debug_argc;
#endif

    if (argc != 4)
        exit(1);

    ifstream is(argv[2], ios::binary);
    if(!is)
        exit(2);

    ofstream os(argv[3], ios::binary);
    if(!os)
        exit(3);

    huffman huff;
    if((string)(argv[1])=="c")
        return huff.encode(is, os);
    else if((string)(argv[1])=="d")
        return huff.decode(is, os);
    else 
        exit(4); 
}