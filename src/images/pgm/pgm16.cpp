#include "pgm16.h"
#include <fstream>
#include <iostream>
#include <vector>

const bool DEBUG=true;

using namespace std;

class pgm16
{
private:
    ifstream& is_;
    uint64_t buffer_ = 0;
    size_t bits_ = 0;
    int width_;
    int height_;

public:
    pgm16(ifstream& is):is_(is){}
    ~pgm16()=default;

    uint32_t read_bit(size_t size = 8)
    {
        while (bits_<size){
            buffer_|=(uint64_t)(is_.get())<<bits_;
            bits_+=8;
        }
        uint32_t val = buffer_&((1ull << size)-1);
        buffer_>>=size;
        bits_ -= size;
        return val;
    }

    int get_height()
    {
        return height_;
    }

    int get_width()
    {
        return width_;
    }

    uint16_t read_maxvalue()
    {
        string maxvalue="";
        uint8_t val;
        do{
            val=read_bit();
            maxvalue+=val;
        }while(val!='\n');
        return atoi(maxvalue.c_str());
    }

    bool read_height()
    {
        string height="";
        uint8_t val;
        do{
            val=read_bit();
            height+=val;
        }while(val!='\n');
        height_=atoi(height.c_str());
        return true;
    }

    bool read_width()
    {
        string width="";
        uint8_t val;
        do{
            val=read_bit();
            width+=val;
        }while(val!=' ');
        width_=atoi(width.c_str());
        return true;
    }

    bool read_optional_comment()
    {
        if(read_bit()!='#'){
            is_.seekg((uint8_t)(-1), ios::ios_base::cur);
            return true;
        }
        uint8_t val;
        do{
            val=read_bit();
        }while(val!='\n');
        return true;
    }

    bool read_header()
    {
        return read_bit(24)==669008  //P5\n
            && read_optional_comment()
            && read_width()
            && read_height();
    }
};

bool load(const std::string& filename, mat<uint16_t>& img, uint16_t& maxvalue)
{
    ifstream is(filename, ios::binary);
    if(!is) exit(3);

    pgm16 p(is);

    if(!
        p.read_header()
    ) exit(4);

    maxvalue = p.read_maxvalue();

    img.resize(p.get_height(), p.get_width());

    if(maxvalue<256){
        for(size_t i=0; i<p.get_height(); i++){
            for(size_t j=0; j<p.get_width(); j++){
                cout << i << ", " << j << endl;
                img(i,j)=p.read_bit();
            }
        }
    }else{
        uint16_t buffer;
        for(size_t i=0; i<p.get_height(); i++){
            for(size_t j=0; j<p.get_width(); j++){
                buffer = 0;
                buffer = (uint8_t)(p.read_bit())&((uint8_t)(p.read_bit())<<8);
                img(i,j)=buffer;
            }
        }
    }
    return true;
}


int main(int argc, char* argv[])
{

    // frog_bin.pgm

    if(DEBUG){
        char* debug_argv[]={
            (char*)"./pgm",
            (char*)"/home/surface/Downloads/frog_bin.pgm",
            nullptr
        };
        int debug_argc = 2;
        argv=debug_argv;
        argc=debug_argc;
    }

    if(argc!=2) exit(1);

    mat<uint16_t> img;
    uint16_t maxvalue;

    if(!
        load(argv[1], img, maxvalue)
    ) exit(2);

    return 0;
}  