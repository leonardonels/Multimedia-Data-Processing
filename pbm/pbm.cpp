#include "pbm.h"
#include <fstream>
#include <iostream>
#include <cmath>

using namespace std;

class BitReader
{
private:
    ifstream& _is;
    uint64_t _buffer;
    size_t _bits;
public:
    BitReader(ifstream& is) : _is(is), _buffer(0), _bits(0) {}
    BitReader() = default;

    uint32_t read_bits(size_t size = 0)
    {
        while(_bits<size){
            _buffer = (_buffer << 8) | (uint64_t)(_is.get() & 0xFF);
            _bits+=8;
        }
        _bits -= size;
        return (_buffer >> (_bits)) & ((1ull << size) - 1);
    }
};

bool BinaryImage::ReadFromPBM(const std::string& filename)
{
    ifstream is(filename, ios::binary);
    if(!is)
        return false;

    BitReader br(is);
    if(br.read_bits(8) != 'P' || br.read_bits(8) != '4' || br.read_bits(8) != 10)
        return false;

    char c;
    string w = "";

    // An optional comment identified by a '#' character followed by any sequence of characters and ending with the '\n' character. 
    // During reading, it is possible to verify if the '#' character is present, otherwise this field is not present.
    c = br.read_bits(8);
    if(c == '#'){
        do{
            c = br.read_bits(8);
        }while(c != 10);
    }else{
        w += c;
    }

    do{
        c = br.read_bits(8);
        if(c != ' ')
            w += c;
    }while(c != ' ');

    this->W = atoi(w.c_str());
    
    string h = "";
    do{
        c = br.read_bits(8);
        if(c != 10)
            h += c;
    }while(c != 10);

    this->H = atoi(h.c_str());

    for(size_t i = 0; i < this->H; i++){
        for (size_t j = 0; j < ceil((float)(this->W)/8); j++){
            this->ImageData.push_back((uint8_t)(br.read_bits(8)));
        }
    }

    return true;
}

Image BinaryImageToImage(const BinaryImage& bimg)
{
    Image img;

    img.H = bimg.H;
    img.W = bimg.W;

    size_t bits = 0;
    uint8_t buffer;
    size_t stride = (size_t)ceil((float)(bimg.W)/8);
    for(size_t i = 0; (int)i < bimg.H; i++){
        size_t s = 0;
        for(size_t j = 0; j < stride; j++){
            buffer = bimg.ImageData[i*stride + j];
            bits += 8;
            while(bits>0){
                if((int)(s)==img.W) break;
                img.ImageData.push_back((uint8_t)((buffer>>(--bits)&1)==1?0:255));
                s++;
            }
        }
        bits = 0;
    }

    return img;
}
