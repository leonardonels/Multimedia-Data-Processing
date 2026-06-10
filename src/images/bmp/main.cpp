#include <iostream>
#include <fstream>
#include <string>
#include <ranges>
#include <map>
#include <cmath>
#include <vector>

using namespace std;

class bmp
{
private:
    ifstream& is_;
    uint64_t buffer_;
    uint32_t size_, w_, h_;
    size_t bits_;
    string id_ = "BM";

    vector<vector<uint32_t>> data;

    struct color_entry{
        uint32_t b, g, r, i;
    };

    uint32_t read_bits(size_t size = 1)
    {
        while(bits_<size){
            buffer_ |= (uint64_t)(is_.get()) << (bits_);
            bits_+=8;
        }
        uint32_t x = buffer_&((1ull<<size)-1);
        buffer_ >>= size;
        bits_ -= size;
        return x;
    }

    uint32_t read_data(size_t size = 1)
    {
        while(bits_<size){
            buffer_ = (buffer_ << 8) | (uint64_t)(is_.get());
            bits_+=8;
        }
        bits_ -= size;
        return (buffer_>>bits_)&((1ull<<size)-1);
    }

    void put_str(ofstream& os, string s)
    {
        size_t size = s.size();
        for(size_t i = 0; i < size; i++){
            os.put(s[i]);
        }
    }

    void put_int(ofstream& os, uint32_t u)
    {
        string s = to_string(u);
        put_str(os, s);
    }

    void goto_offset(uint32_t offset){
        is_.seekg(offset, ios::beg);
        buffer_ = 0;
        bits_ = 0;
    }

public:
    bmp(ifstream& is) : is_(is), buffer_(0), bits_(0), size_(0), w_(0), h_(0) {}
    ~bmp() = default;

    void to_pam(ofstream& os)
    {
        // read bitmap file header        
        uint16_t id = read_bits(16);
        for(auto i : ranges::iota_view(0, 2)){
            if(((id>>(i*8))&0xFF)!=id_[i])
                exit(4);
        }
        size_ = read_bits(32);
        read_bits(32);  // reserved
        uint32_t offset = read_bits(32);

        // bitmap info header
        uint32_t h_size = read_bits(32);
        w_ = read_bits(32);
        h_ = read_bits(32);
        uint16_t color_planes = read_bits(16);
        uint16_t bpp = read_bits(16);   // bit per pixel
        uint32_t compression = read_bits(32);
        uint32_t raw_size = read_bits(32);  // the image size. This is the size of the raw bitmap data; a dummy 0 can be given for BI_RGB bitmaps.
        uint32_t h_res = read_bits(32); // pixel per meter, signed integer
        uint32_t v_res = read_bits(32); // pixel per meter, signed integer
        uint32_t num_colors = read_bits(32);    // the number of colors in the color palette, or 0 to indicate 2ⁿ, with n the numeber of bits per pixel
        uint32_t i_colors = read_bits(32);  // the number of important colors used, or 0 when every color is important; generally ignored
        
        map<uint8_t, color_entry> color_table;
        if(bpp  != 24){
            if(num_colors == 0)
                num_colors=1u << bpp;

            // color table
            // cout << "num_colors: " << num_colors << endl;
            for(size_t i = 0; i < num_colors; i++){
                color_entry e;
                e.b = read_bits(8);
                e.g = read_bits(8);
                e.r = read_bits(8);
                e.i = read_bits(8);
                color_table[i] = e;
                // cout << e.r << ", " << e.g << ", " << e.b << ", " << e.i << endl;
            }
        }

        goto_offset(offset);    // probably unnecessary
        
        // load image
        uint32_t row_bits     = w_ * bpp;
        uint32_t padded_bits  = ((row_bits + 31) / 32) * 32;
        uint32_t padding_bits = padded_bits - row_bits;

        vector<vector<color_entry>> raw_table;

        for (size_t i = 0; i < h_; i++) {
            if(bpp == 24){
                vector<color_entry> v;
                for (size_t j = 0; j < w_; j++) {
                    color_entry e;
                    e.b = read_data(8);
                    e.g = read_data(8);
                    e.r = read_data(8);
                    e.i = 0;
                    v.push_back(e);
                    // cout << e.r << ", " << e.g << ", " << e.b << endl;
                }
                if (padding_bits) read_data(padding_bits);   // discard padding
                raw_table.emplace_back(move(v));
            }else{
                vector<uint32_t> row;
                row.reserve(w_);
                for (size_t j = 0; j < w_; j++) {
                    row.push_back(read_data(bpp));
                }
                if (padding_bits) read_data(padding_bits);
                data.emplace_back(move(row));
            }
        }

        // PAM configuration
        put_str(os, "P7");
        put_str(os, "\nWIDTH ");
        put_int(os, w_);
        put_str(os, "\nHEIGHT ");
        put_int(os, h_);
        put_str(os, "\nDEPTH 3");
        put_str(os, "\nMAXVAL 255");
        put_str(os, "\nTUPLTYPE RGB");
        put_str(os, "\nENDHDR\n");

        if(bpp == 24){
            reverse(raw_table.begin(), raw_table.end());
            for(auto r : raw_table){
                for(auto e : r){
                    os.put(e.r);
                    os.put(e.g);
                    os.put(e.b);
                }
            }
        }else{
            reverse(data.begin(), data.end());
            for(auto r : data){
                for(auto c : r){
                    color_entry e = color_table[c];
                    os.put(e.r);
                    os.put(e.g);
                    os.put(e.b);
                }
            }
        }
    }
};

int main(int argc, char* argv[])
{
    #ifdef DEBUG
    argc = 3;
    char* d_argv[] = {
        (char*)"./bmp2pam",
        (char*)"./rana24.bmp",
        (char*)"./rana24.pam",
        nullptr
    };
    argv = d_argv;
    #endif

    if(argc != 3)
        exit(1);

    ifstream is(argv[1], ios::binary);
    if(!is)
        exit(2);
    
    ofstream os(argv[2], ios::binary);
    if(!os)
        exit(3);

    bmp bmp(is);
    bmp.to_pam(os);
    
    return 0;
}