#include <fstream>
#include <iostream>
#include <cstdint>
#include <ranges>
#include <string>
#include <vector>

#define QOI_OP_RGB (uint8_t)254
#define QOI_OP_RGBA (uint8_t)255

#define QOI_OP_INDEX (uint8_t)0
#define QOI_OP_DIFF (uint8_t)1
#define QOI_OP_LUMA (uint8_t)2
#define QOI_OP_RUN (uint8_t)3

using namespace std;

struct pixel{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

struct qoi_header {
    char magic[4];
    uint32_t width;
    uint32_t height;
    uint8_t channels;   // 3 = RGB, 4 = RGBA
    uint8_t colorspace; // 0 = sRGB with linear alpha
                        // 1 = all channels linear
};

uint32_t read_uint32_t(ifstream& is)
{
    uint64_t buffer = 0;
    size_t bits = 0;
    while(bits < 32){
        buffer = (buffer << 8) | uint64_t(is.get()&0xFF);
        bits += 8;
    }
    return buffer&0xFFFFFFFF;
}

uint8_t read_uint8_t(ifstream& is)
{
    return is.get();
}

qoi_header read_header(ifstream& is)
{
    qoi_header header;
    for(auto i : ranges::iota_view(0,4)){
        header.magic[i] = read_uint8_t(is);
    }
    header.width = read_uint32_t(is);
    header.height = read_uint32_t(is);
    header.channels = read_uint8_t(is);
    header.colorspace = read_uint8_t(is);
    return header;
}

uint8_t qoi_hash(const pixel& px) {
    return (px.r * 3 + px.g * 5 + px.b * 7 + px.a * 11) & 0x3F;
}

void load_img(ifstream& is, qoi_header& header, vector<pixel>& img)
{
    vector<pixel> index(64, {0,0,0,0});
    pixel prev = {0,0,0,255};

    // for each chunk
    size_t size = header.height*header.width;
    size_t i = 0;
    for(;i<size;){
        pixel p;
        uint8_t x, run = 0;
        uint8_t tag = read_uint8_t(is);
        int8_t dg, db, dr;
        switch (tag)
        {
        case QOI_OP_RGB:{
            p.r = read_uint8_t(is);
            p.g = read_uint8_t(is);
            p.b = read_uint8_t(is);
            p.a = prev.a;
            img.push_back(p); i++;
            index[qoi_hash(p)] = p;
            prev = p;
            break;
        }
        case QOI_OP_RGBA:{
            p.r = read_uint8_t(is);
            p.g = read_uint8_t(is);
            p.b = read_uint8_t(is);
            p.a = read_uint8_t(is);
            img.push_back(p); i++;
            index[qoi_hash(p)] = p;
            prev = p;
            break;
        }
        default:{
            switch ((tag>>6)&0x3)
            {
            case QOI_OP_INDEX:{
                uint8_t idx = tag&0x3F;
                img.push_back(index[idx]); i++;
                prev = index[idx];
                break;
            }
            case QOI_OP_RUN:{
                run = (tag&0x3F) + 1;
                for(auto j : ranges::iota_view(0, int(run))){
                    img.push_back(prev); i++;
                }
                break;
            }
            case QOI_OP_LUMA:{
                dg = (tag&0x3F) - 32;
                x = read_uint8_t(is);
                dr = ((x>>4)&0xF) - 8 + dg;
                db = ((x&0xF) - 8) + dg;
                p.r = uint8_t(prev.r + dr);
                p.g = uint8_t(prev.g + dg);
                p.b = uint8_t(prev.b + db);
                p.a = prev.a;
                img.push_back(p); i++;
                index[qoi_hash(p)] = p;
                prev = p;
                break;
            }
            case QOI_OP_DIFF:{
                dr = int8_t((tag >> 4) & 0x3) - 2;
                dg = int8_t((tag >> 2) & 0x3) - 2;
                db = int8_t(tag & 0x3) - 2;
                p.r = uint8_t(prev.r + dr);
                p.g = uint8_t(prev.g + dg);
                p.b = uint8_t(prev.b + db);
                p.a = prev.a;
                img.push_back(p); i++;
                index[qoi_hash(p)] = p;
                prev = p;
                break;
            }
            };
        };
        };
    }
}

void write_img(qoi_header header, vector<pixel>& pixel, ofstream& os)
{
    os.write("P7", 2);
    os.write("\nWIDTH ", 7);
    string width = to_string(header.width);
    for(auto i : ranges::iota_view(0, (int)width.length())){
        os.put(width[i]);
    }
    os.write("\nHEIGHT ", 8);
    string height = to_string(header.height);
    for(auto i : ranges::iota_view(0, (int)height.length())){
        os.put(height[i]);
    }
    os.write("\nDEPTH 4", 8);
    os.write("\nMAXVAL 255", 11);
    os.write("\nTUPLTYPE RGB_ALPHA", 19);
    os.write("\nENDHDR\n", 8);
    
    for(auto p : pixel){
        os.put(p.r);        
        os.put(p.g);
        os.put(p.b);
        os.put(p.a);        
    }
}

void qoi_decomp(string infile, string outfile)  // to PAM with always RGBA pixel format
{
    ifstream is(infile, ios::binary);
    if(!is)
        exit(2);

    ofstream os(outfile, ios::binary);
    if(!os)
        exit(3);

    qoi_header header = read_header(is);

    vector<pixel> img;
    load_img(is, header, img);

    write_img(header, img, os);

    // decoder start variables
    uint8_t prev_rgba[4] = {0,0,0,255};
}

int main(int argc, char* argv[])
{
    #ifdef DEBUG
    char* d_argv[] = {
        (char*)"./main",
        (char*)"./kodim10.qoi",
        (char*)"./output_file.pam",
        nullptr
    };
    argv = d_argv;
    argc = 3;
    #endif

    if(argc!=3)
        exit(1);

    qoi_decomp(argv[1], argv[2]);
    
    return EXIT_SUCCESS;
}