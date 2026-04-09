#include <iostream>
#include <fstream>
#include <stdint.h>
#include <bit>
#include <vector>

static const uint32_t MAGICNUMBER = 0x184C2103;

using namespace std;

class lz4decomp
{
private:
    ifstream& is_;
    ofstream& os_;
    uint32_t buffer_;
    vector<uint8_t> output_buffer_;

public:
    lz4decomp(ifstream& is, ofstream& os) : is_(is), os_(os), buffer_(0){}
    ~lz4decomp(){

    }

    uint32_t getBuffer(){return buffer_;}

    uint8_t writeLiteral()
    {
        uint8_t literal=readBuffer(1);
        os_.put(literal);
        output_buffer_.push_back(literal);
        return literal;
    }

    void writeMatch(uint16_t offset, uint8_t match_len)
    {
        size_t start_pos = output_buffer_.size() - offset;
        for(size_t i=0;i<match_len;i++){
            uint8_t byte = output_buffer_[start_pos + i];
            os_.put(byte);
            output_buffer_.push_back(byte);
            cout << byte; // for debugging
        }
    }

    uint32_t readBuffer(size_t n=4)
    {
        is_.read((char*)&buffer_, sizeof(uint8_t) * n);
        return buffer_;
    }

    uint32_t readHeader()
    {
        if(readBuffer() != MAGICNUMBER){
            cerr << "Not a valid LZ4 file" << endl;
            exit(3);
        }
        uint32_t uncompressed_size = readBuffer();
        if(readBuffer() != 0x4D000000){
            cerr << "Unsupported LZ4 format (compressed size not supported)" << endl;
            exit(4);
        }
        return uncompressed_size;
    }

    tuple<uint8_t, uint8_t> readToken()
    {
        readBuffer(1);
        return tuple<uint8_t, uint8_t>(buffer_>>4, (buffer_&0xF)+4);
    }
};

void decode(lz4decomp& ld)
{
    size_t reads=0;
    size_t writes=0;
    uint32_t uncompressed_size = ld.readHeader();
    uint32_t block_size=ld.readBuffer();
    while(reads < block_size){
        auto [literal_len,match_len]=ld.readToken(); reads++;
        if(literal_len==15){
            do{
                literal_len+=ld.readBuffer(1); reads++;
            }while(ld.getBuffer()==255);
        }
        for(size_t i=0;i<literal_len;i++){
            cout << // for debugging
            ld.writeLiteral(); reads++; writes++;
        }
        uint16_t offset=ld.readBuffer(2); reads+=2;
        if(uncompressed_size==writes) break;
        if(match_len==19){
            do{
                match_len+=ld.readBuffer(1); reads++;
            }while(ld.getBuffer()==255);
        }
        ld.writeMatch(offset, match_len); writes+=match_len;
    }
    cout << "\nUncompressed size: " << uncompressed_size << ", Writes: " << writes << endl;
}

int main(int argc, char* argv[])
{
    const char* input_file = "/home/surface/Downloads/example1.txt.lz4";
    const char* output_file = "/home/surface/Downloads/example1_decompressed.txt";

    bool debug = true;
    if(!debug){
        if(argc != 3){
            cerr << "Usage: " << argv[0] << " <input_file> <output_file>" << endl;
            return 1;
        }
        input_file = argv[1];
        output_file = argv[2];
    }

    ifstream is(input_file, ios::binary);
    if(!is) return 1;
    ofstream os(output_file);
    if(!os) return 2;

    lz4decomp ld(is, os);
    
    decode(ld);

    return 0;
}