#include <cstdint>
#include <fstream>
#include <iostream>
#include <ranges>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

using namespace std;

struct pixel{
    uint16_t  r;
    uint16_t  g;
    uint16_t  b;
};

struct row{
    vector<pixel> data;
};

class PGM
{   
private:
    ifstream& is_;

    uint64_t buffer_ = 0;
    size_t bits_ = 0;

public:
    uint32_t width_, height_, maxval_;
    vector<row> data_;

    PGM(ifstream& is) : is_(is) {}
    ~PGM() = default;

    uint16_t uread16_t()    // MSB Fisrt
    {
        while(bits_ < 16){
            buffer_ = (buffer_<<8) | is_.get();
            bits_ += 8;
        }

        bits_-=16;
        return (buffer_>>bits_)&0xFFFF;
    }

    uint8_t uread8_t()    // MSB Fisrt
    {
        while(bits_ < 8){
            buffer_ = (buffer_<<8) | is_.get();
            bits_ += 8;
        }

        bits_-=8;
        return (buffer_>>bits_)&0xFF;
    }

    uint32_t readVal()
    {
        char x = 0;
        string y = "";
        do{
            x = uread8_t();
            y += x;
        }while((x != '\n') && (x != ' '));
        return (uint32_t)atoi(y.c_str());
    }

    uint16_t sat16(double v){
        if (v < 0.0) return 0;
        if (v > 65535.0) return 65535;
        return (uint16_t)round(v);
    };

    void load(string prefix)
    {
        string output_pgm = prefix + "_gray.pgm";
        string output_ppm = prefix + "_bayer.ppm";
        string output_green = prefix + "_green.ppm";
        string final_ppm = prefix + "_interp.ppm";
        ofstream os_pgm(output_pgm, ios::binary);
        if(!os_pgm)
            exit(2);
        ofstream os_bayer(output_ppm, ios::binary);
        if(!os_bayer)
            exit(3);
        ofstream os_green(output_green, ios::binary);
        if(!os_green)
            exit(4);
        ofstream os_final(final_ppm, ios::binary);
        if(!os_final)
            exit(5);
        
        // header in
        string magicnumber = "P5\n";
        for(auto i : ranges::iota_view(0, 3)){
            uint8_t c = uread8_t();
            if(magicnumber[i] != c)
                exit(10);
        }
        width_ = readVal();
        height_ = readVal();
        maxval_ = readVal();
        // cout << width_ << " x " << height_ << " - " << maxval_ << endl;

        // out pgm
        os_pgm.write("P5 ", 3);
        for(auto c : to_string(width_)){
            os_pgm.put(c);
        }
        os_pgm.put(' ');
        for(auto c : to_string(height_)){
            os_pgm.put(c);
        }
        os_pgm.put(' ');
        os_pgm.write("255 ", 4);

        // out ppm
        os_bayer.write("P6 ", 3);
        for(auto c : to_string(width_)){
            os_bayer.put(c);
        }
        os_bayer.put(' ');
        for(auto c : to_string(height_)){
            os_bayer.put(c);
        }
        os_bayer.put(' ');
        os_bayer.write("255\n", 4);

        // out green
        os_green.write("P6 ", 3);
        for(auto c : to_string(width_)){
            os_green.put(c);
        }
        os_green.put(' ');
        for(auto c : to_string(height_)){
            os_green.put(c);
        }
        os_green.put(' ');
        os_green.write("255\n", 4);

        // out final
        os_final.write("P6 ", 3);
        for(auto c : to_string(width_)){
            os_final.put(c);
        }
        os_final.put(' ');
        for(auto c : to_string(height_)){
            os_final.put(c);
        }
        os_final.put(' ');
        os_final.write("255\n", 4);

        // data
        for(auto i : ranges::iota_view(0, int(height_))){
            row r;
            for(auto j : ranges::iota_view(0, int(width_))){
                if((i%2)&&(j%2)){     // B
                    uint16_t x = uread16_t();
                    r.data.push_back({0,0,x});
                    os_pgm.put(uint8_t(x/256));

                    os_bayer.put(0);              // R
                    os_bayer.put(0);              // G
                    os_bayer.put(uint8_t(x/256));     // B
                    continue;
                }
                if((!(i%2))&&(!(j%2))){   // R
                    uint16_t x = uread16_t();
                    r.data.push_back({x,0,0});
                    os_pgm.put(uint8_t(x/256));

                    os_bayer.put(uint8_t(x/256));     // R
                    os_bayer.put(0);             // G
                    os_bayer.put(0);              // B
                    continue;
                }                   // G
                uint16_t x = uread16_t();
                r.data.push_back({0,x,0});
                os_pgm.put(uint8_t(x/256));

                os_bayer.put(0);                  // R
                os_bayer.put(uint8_t(x/256));         // G
                os_bayer.put(0);                  // B
            }
            data_.push_back(r);
        }

        // green interpolation
        for(auto i : ranges::iota_view(0, int(height_))){
            for(auto j : ranges::iota_view(0, int(width_))){
                if(data_[i].data[j].g != 0)
                    continue;

                int X1, G2, X3, G4, X5, G6, X7, G8, X9;
                G2 = (i-1) >= 0 ? (int)data_[i-1].data[j].g : 0;
                G4 = (j-1) >= 0 ? (int)data_[i].data[j-1].g : 0;
                G6 = (j+1) < width_ ? (int)data_[i].data[j+1].g : 0;
                G8 = (i+1) < height_ ? (int)data_[i+1].data[j].g : 0;

                if((i%2)&&(j%2)){     // B
                    X1 = (i-2) >= 0 ? (int)data_[i-2].data[j].b : 0;
                    X3 = (j-2) >= 0 ? (int)data_[i].data[j-2].b : 0;
                    X5 = (int)data_[i].data[j].b;                
                    X7 = (j+2) < width_ ? (int)data_[i].data[j+2].b : 0;
                    X9 = (i+2) < height_ ? (int)data_[i+2].data[j].b : 0;
                }
                if((!(i%2))&&(!(j%2))){   // R
                    X1 = (i-2) >= 0 ? (int)data_[i-2].data[j].r : 0;
                    X3 = (j-2) >= 0 ? (int)data_[i].data[j-2].r : 0;
                    X5 = (int)data_[i].data[j].r;                
                    X7 = (j+2) < width_ ? (int)data_[i].data[j+2].r : 0;
                    X9 = (i+2) < height_ ? (int)data_[i+2].data[j].r : 0;
                }                   // G

                int H = abs(G4 - G6) + abs(X5 - X3 + X5 - X7);
                int V = abs(G2 - G8) + abs(X5 - X1 + X5 - X9);
                
                double green;
                if(H<V){
                    green = ((G4+G6)*0.5 + (X5-X3+X5-X7)*0.25);
                }else if (H>V){
                    green = (G2+G8)*0.5 + (X5-X1+X5-X9)*0.25;
                }else{
                    green = (G2+G4+G6+G8)*0.25 + (X5-X1+X5-X3+X5-X7+X5-X9)*0.125;
                }
                data_[i].data[j].g = sat16(green);
            }
        }
        for(auto i : ranges::iota_view(0, int(height_))){
            for(auto j : ranges::iota_view(0, int(width_))){
                os_green.put(0);                                      // R
                os_green.put((char)round(data_[i].data[j].g/256.0));    // G
                os_green.put(0);                                      // B
            }
        }

        for(auto i : ranges::iota_view(0, int(height_))){
            for(auto j : ranges::iota_view(0, int(width_))){
                int X1, X3, X7, X9, G1, G3, G5, G7, G9; 
                G1 = (i-1) >= 0 ? ((j-1) >= 0 ? (int)data_[i-1].data[j-1].g : 0) : 0;
                G3 = (i-1) >= 0 ? ((j+1) < width_ ? (int)data_[i-1].data[j+1].g : 0) : 0;
                G5 = (int)data_[i].data[j].g;
                G7 = (i+1) < height_ ? ((j-1) >= 0 ? (int)data_[i+1].data[j-1].g : 0) : 0;
                G9 = (i+1) < height_ ? ((j+1) < width_ ? (int)data_[i+1].data[j+1].g : 0) : 0;

                if((!(i%2))&&(!(j%2))){ // R
                    X1 = (i-1) >= 0 ? ((j-1) >= 0 ? (int)data_[i-1].data[j-1].b : 0) : 0;
                    X3 = (i-1) >= 0 ? ((j+1) < width_ ? (int)data_[i-1].data[j+1].b : 0) : 0;
                    X7 = (i+1) < height_ ? ((j-1) >= 0 ? (int)data_[i+1].data[j-1].b : 0) : 0;
                    X9 = (i+1) < height_ ? ((j+1) < width_ ? (int)data_[i+1].data[j+1].b : 0) : 0;
                    
                    int N = abs(X1 - X9) + abs(G5 - G1 + G5 - G9);
                    int P = abs(X3 - X7) + abs(G5 - G3 + G5 - G7);
                    if(N < P){
                        data_[i].data[j].b = sat16((X1 + X9)*0.5 + (G5 - G1 + G5 - G9)*0.25);
                    }else if (N > P){
                        data_[i].data[j].b = sat16((X3 + X7)*0.5 + (G5 - G3 + G5 - G7)*0.25);
                    }else{  // N == P
                        data_[i].data[j].b = sat16((X1 + X3 + X7 + X9)*0.25 + (G5 - G1 + G5 - G3 + G5 - G7 + G5 - G9)*0.125);
                    }
                    continue;
                }   

                if((i%2)&&(j%2)){   // B
                    X1 = (i-1) >= 0 ? ((j-1) >= 0 ? (int)data_[i-1].data[j-1].r : 0) : 0;
                    X3 = (i-1) >= 0 ? ((j+1) < width_ ? (int)data_[i-1].data[j+1].r : 0) : 0;
                    X7 = (i+1) < height_ ? ((j-1) >= 0 ? (int)data_[i+1].data[j-1].r : 0) : 0;
                    X9 = (i+1) < height_ ? ((j+1) < width_ ? (int)data_[i+1].data[j+1].r : 0) : 0;

                    int N = abs(X1 - X9) + abs(G5 - G1 + G5 - G9);
                    int P = abs(X3 - X7) + abs(G5 - G3 + G5 - G7);
                    if(N < P){
                        data_[i].data[j].r = sat16((X1 + X9)*0.5 + (G5 - G1 + G5 - G9)*0.25);
                    }else if (N > P){
                        data_[i].data[j].r = sat16((X3 + X7)*0.5 + (G5 - G3 + G5 - G7)*0.25);
                    }else{  // N == P
                        data_[i].data[j].r = sat16((X1 + X3 + X7 + X9)*0.25 + (G5 - G1 + G5 - G3 + G5 - G7 + G5 - G9)*0.125);
                    }
                    continue;
                }   

                // G
                if(!(i%2)){    // R G R    
                    int R1 = (j-1) >= 0 ? (int)data_[i].data[j-1].r : 0;
                    int R2 = (j+1) < width_ ? (int)data_[i].data[j+1].r : 0;
                    data_[i].data[j].r = sat16((R1 + R2)*0.5);

                    int B1 = (i-1) >= 0 ? (int)data_[i-1].data[j].b : 0;
                    int B2 = (i+1) < height_ ? (int)data_[i+1].data[j].b : 0;
                    data_[i].data[j].b = sat16((B1 + B2)*0.5);
                }else{      // B G B
                    int B1 = (j-1) >= 0 ? (int)data_[i].data[j-1].b : 0;
                    int B2 = (j+1) < width_ ? (int)data_[i].data[j+1].b : 0;
                    data_[i].data[j].b = sat16((B1 + B2)*0.5);

                    int R1 = (i-1) >= 0 ? (int)data_[i-1].data[j].r : 0;
                    int R2 = (i+1) < height_ ? (int)data_[i+1].data[j].r : 0;
                    data_[i].data[j].r = sat16((R1 + R2)*0.5);
                }
                continue;
            }
        }
        for(auto i : ranges::iota_view(0, int(height_))){
            for(auto j : ranges::iota_view(0, int(width_))){
                os_final.put(data_[i].data[j].r/256);    // R
                os_final.put(data_[i].data[j].g/256);    // G
                os_final.put(data_[i].data[j].b/256);    // B
            }
        }
    }
};

void bayer_decode(string input_file, string prefix)
{
    ifstream is(input_file, ios::binary);
    if(!is)
        exit(2);

    PGM pgm(is);
    pgm.load(prefix);
}

int main(int argc, char* argv[])
{
    #ifdef DEBUG
    char* d_argv[] = {
        (char*)"./main",
        (char*)"D5100LL004003.pgm",     // "D5100LL004003.pgm",
        (char*)"D5100LL004003",
        nullptr
    };
    argv = d_argv;
    argc = 3;
    #endif

    if(argc != 3)
        exit(1);

    bayer_decode(argv[1], argv[2]);

    return 0;
}