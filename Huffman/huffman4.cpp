// Same as huffman2.cpp (Canonical)

#include <stdexcept>
#include <fstream>
#include <string>
#include <unordered_map>
#include <queue>
#include <iostream>

using namespace std;

const string MAGICNUMBER = "HUFFMAN4";

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

    struct canonical_code
    {
        uint8_t len_;
        uint8_t sym_;
        canonical_code(uint8_t len, uint8_t sym) : sym_(sym), len_(len) {}
    };

    struct freq_cmp
    {
        bool operator()(const node* a, const node* b) const{
            return a->frequency_ > b->frequency_;
        }
    };

    struct code_cmp
    {
        bool operator()(const canonical_code* a, const canonical_code* b) const{
            if(a->len_ != b->len_) return a->len_ > b->len_;
            return a->sym_ > b->sym_;
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

    void build_codes(const node* n, uint8_t len, priority_queue<canonical_code*, vector<canonical_code*>, code_cmp>& codes) const
    {
        if(!n) return;
        if(!n->left_ && !n->right_){
            codes.push(new canonical_code(len == 0? 1 : len, n->symbol_));
            return;
        }
        build_codes(n->left_, ++len, codes);    // len + 1
        build_codes(n->right_, len, codes);   // len + 1
    }

    unordered_map<uint8_t, pair<uint8_t, uint32_t>> build_table(priority_queue<canonical_code*, vector<canonical_code*>, code_cmp>& codes, BitWriter& bw)
    {
        unordered_map<uint8_t, pair<uint8_t, uint32_t>> table;

        bool first_code = true;
        uint8_t prev_len = 0;
        uint32_t prev_code = 0;
        while(codes.size() > 0){
            canonical_code* entry = codes.top(); codes.pop();
            uint32_t code;
            if (first_code) {
                code = 0;
                first_code = false;
            } else {
                code = (prev_code + 1) << (entry->len_ - prev_len);
            }
            table[entry->sym_] = {entry->len_, code};
            prev_len = entry->len_;
            prev_code = code;

            bw.write_bits(entry->sym_, 8);
            bw.write_bits(entry->len_, 5);

            delete entry;
        }

        return table;
    }

    unordered_map<uint64_t, uint8_t> build_table(priority_queue<canonical_code*, vector<canonical_code*>, code_cmp>& codes)
    {
        unordered_map<uint64_t, uint8_t> table;

        bool first_code = true;
        uint8_t prev_len = 0;
        uint32_t prev_code = 0;
        while(codes.size() > 0){
            canonical_code* entry = codes.top(); codes.pop();
            uint32_t code;
            if (first_code) {
                code = 0;
                first_code = false;
            } else {
                code = (prev_code + 1) << (entry->len_ - prev_len);
            }
            table[make_key(entry->len_, code)] = entry->sym_;
            prev_len = entry->len_;
            prev_code = code;

            delete entry;
        }

        return table;
    }

    int encode(ifstream& is, ofstream& os)
    {
        unordered_map<uint8_t, uint32_t> frequency;
        freq_count(is, frequency);

        node* root = build_tree(frequency);

        // [MAGICNUMBER]
        BitWriter bw(os);
        for(const auto& c : MAGICNUMBER){
            bw.write_bits(c, 8);
        }

        // [TABLE ENTRIES]
        bw.write_bits(frequency.size()!=256 ? frequency.size() : 0, 8);

        priority_queue<canonical_code*, vector<canonical_code*>, code_cmp> codes;
        build_codes(root, 0, codes);
        delete root;

        // [TABLE]
        unordered_map<uint8_t, pair<uint8_t, uint32_t>> table = build_table(codes, bw);

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
            pair<uint8_t, uint32_t> entry = table[c];
            uint32_t code = entry.second;
            uint8_t  len  = entry.first;
            for (int i = len - 1; i >= 0; i--)
                bw.write_bits((code >> i) & 1, 1);
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
        priority_queue<canonical_code*, vector<canonical_code*>, code_cmp> codes;
        for (size_t i = 0; i<entries; i++){
            uint8_t sym = br.read_bits(8);
            uint8_t len = br.read_bits(5);
            codes.push(new canonical_code(len, sym));
        }

        return build_table(codes);
    }

    int decode(ifstream& is, ofstream& os)
    {
        // [MAGICNUMBER]
        BitReader br(is);
        for (char expected : MAGICNUMBER) {
            uint8_t got = br.read_bits(8);
            if (got != (uint8_t)expected) exit(5);
        }
        
        // [TABLE ENTRIES]
        // [TABLE]
        unordered_map<uint64_t, uint8_t> table = read_table(br);

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
            if (table.find(make_key(len, c)) != table.end()){
                bw.write_bits(table[make_key(len, c)], 8);
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