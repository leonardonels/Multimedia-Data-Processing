#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <stdint.h>
#include <map>
#include <cmath>

using namespace std;

const string MAGICNUMBER = "LZ78";

class bitwriter
{
private:
    uint8_t buffer_;
    size_t bits_;
    ofstream& os_;

public:
    bitwriter(ofstream& os) : buffer_(0), bits_(0), os_(os) {};
    ~bitwriter(){
        if (bits_ > 0) {
            buffer_ <<= (8 - bits_);
            os_.put(static_cast<char>(buffer_));
        }
    }

    void clean(){buffer_=0; bits_=0;}

    void write(){        
        os_.put(static_cast<char>(buffer_));
        clean();
    }

    // lz78 -> (dict_idx, char)
    void bitwrite(const char input){
        for(size_t i=0;i<8;i++){
            buffer_ <<= 1;
            buffer_ |= (input>>(7-i))&1;
            bits_++;

            if(bits_==8){
                write();
            }
        }
    }

    void bitwrite(const uint8_t input, size_t size){
        if(size==0)return;

        uint8_t n=(uint8_t)floor(log2((double)size))+1;
        uint8_t val=input&((1u<<n)-1);
        for(size_t i=0;i<n;i++){
            buffer_ <<= 1;
            buffer_ |= (val>>(n-i-1))&1;
            bits_++;

            if(bits_==8){
                write();
            }
        }
    }

    // maxbits with 5 bits
    void bitwrite(const int maxbits){
        buffer_ <<= 5;
        buffer_ |= (static_cast<uint8_t>(maxbits)&0x1F);
        bits_ += 5;
    }
};

bool lz78encode(const std::string& input_filename, const std::string& output_filename, int maxbits_){

    ifstream is(input_filename, ios::binary);
    if(!is){return false;}

    ofstream os(output_filename, ios::binary);
    if(!os){return false;}

    bitwriter bw(os);

    os.write(MAGICNUMBER.c_str(), 4*sizeof(char));

    bw.bitwrite(maxbits_);

    string buffer="";
    size_t size=0;
    char c;
    map<string, uint8_t> dict;
    while(is >> c){
        string next = buffer + c;
        if(size==(((1u<<maxbits_)-1)+1)){
            dict.clear();
            size=0;
        }
        if(dict.find(next)==dict.end()){
            bw.bitwrite(dict[buffer], size);
            bw.bitwrite(c);
            dict[next]=++size;
            buffer="";
        }else{
            buffer=next;
        }
    }
    if(buffer!=""){
        buffer.pop_back();
        bw.bitwrite(dict[buffer], size);
        bw.bitwrite(c);
    }
    
    return true;
}

int main (int argc, char* argv[]) {

    const char* input_debug = "/home/surface/Downloads/in_test.txt";
    const char* output_debug = "/home/surface/Downloads/out_test.txt";
    int maxbits__debug = 4;

    lz78encode(input_debug, output_debug, maxbits__debug);
    return 0;

//    if (argc != 4) {
//        std::cerr << "Usage: " << argv[0] << " <input_filename> <output_filename> <maxbits_>" << std::endl;
//        return 1;
//    }
//
//    string input_filename = argv[1];
//    string output_filename = argv[2];
//    int maxbits_ = atoi(argv[3]);
//
//    if(maxbits_ < 0 || maxbits_ > 30){
//        cerr << "Wrong maxbits_!\n";
//        return 1;
//    }
//    
//    lz78encode(input_filename, output_filename, maxbits_);
//
//    return 0;
}