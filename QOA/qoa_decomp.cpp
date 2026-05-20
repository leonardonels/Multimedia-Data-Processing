#include <fstream>
#include <iostream>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <ranges>
#include <algorithm>

using namespace std;

class bitReader
{
private:
    ifstream& is_;

    uint64_t buffer_ = 0;
    size_t bits_ = 0;
public:
    bitReader(ifstream& is):is_(is){};
    ~bitReader()=default;

    uint64_t readBits(size_t size = 8)  // big endian
    {
        while(bits_ < size){
            buffer_ = (buffer_ << 8) | uint8_t(is_.get()&0xFF);
            bits_ += 8;
        }

        bits_ -= size;
        return uint64_t((buffer_ >> bits_)&((1ull<<size)-1)) ;
    }
};


void qoa_to_wav(char* input_file, char* output_file)
{
    ifstream is(input_file, ios::binary);
    if(!is)
        exit(2);
    
    ofstream os(output_file, ios::binary);
    if(!os)
        exit(3);

    bitReader br(is);

    // header
    string QOA = "qoaf";

    for(size_t i = 0; i < QOA.size(); i++){
        if(br.readBits() != QOA[i])
            exit(4);
    }

    uint32_t num_samples = br.readBits(32);

    vector<uint16_t> samples_L;
    vector<uint16_t> samples_R;


    // ceil(samples/(256*20)) frames
    double dequant_tab[] = {0.75, -0.75, 2.5, -2.5, 4.5, -4.5, 7, -7};
    for(size_t i = 0; i < ceil((double)num_samples/(256*20)); i++){
        // cout << i << " out of " << ceil((double)num_samples/(256*20)) << endl;
        // frame header
        uint8_t num_channels = br.readBits();
        uint32_t samplerate = br.readBits(24);
        uint16_t fsamples = br.readBits(16);
        uint16_t fsize = br.readBits(16);
        // lms_state
        int16_t history[2][4], weights[2][4];
        for(auto j : ranges::iota_view(0, int(num_channels))){
            for(auto k : ranges::iota_view(0, 4)){
                history[j][k] = br.readBits(16);
            }
            for(auto k : ranges::iota_view(0, 4)){
                weights[j][k] = br.readBits(16);
            }
        }
        // num_channels*256 slices
        size_t num_slices = num_channels*256;
        for(auto j : ranges::iota_view(0, int(num_slices))){
            // read one slice -> 8 bytes -> 20 samples
            
            uint8_t sf_quant = br.readBits(4);
            
            vector<uint8_t> qrNN(20);
            for(auto k : ranges::iota_view(0,20)){
                qrNN[k]=br.readBits(3);
            }

            double sf = round(pow(sf_quant + 1, 2.75));
            size_t q = 0;
            for(auto qr : qrNN){
                double r = sf * dequant_tab[qr];
                r = (r < 0) ? ceil(r - 0.5) : floor(r + 0.5);
                
                int64_t p = 0;
                for (int n = 0; n < 4; ++n)
                    p += int64_t(history[j%num_channels][n]) * int64_t(weights[j%num_channels][n]);
                p >>= 13;
                int r_int = r;
                int s = int(p) + r_int;
                s = clamp(s, (int)INT16_MIN, (int)INT16_MAX);

                int ch = j % num_channels;
                auto& buf = (ch == 0) ? samples_L : samples_R;
                if (buf.size() < num_samples)
                    buf.push_back(uint16_t(s));
                // cout << "sample: " << samples.size() << " out of " << num_samples*num_channels << endl;
                
                int32_t delta = int32_t(r) >> 4;
                for (auto n : ranges::iota_view(0,4)){
                    weights[j%num_channels][n] += history[j%num_channels][n] < 0 ? -delta : delta;
                }

                for (auto n : ranges::iota_view(0, 3)){
                    history[j%num_channels][n] = history[j%num_channels][n+1];
                }
                history[j%num_channels][3] = s;
            }
        }
    }

    // wav write
    uint32_t val;
    uint16_t h_val;

    // riff
    os.write("RIFF", 4);
    // file size placeholder
    os.write("PHLD", 4);
    // type header
    os.write("WAVE", 4);
    // chunk marker
    os.write("fmt ", 4);
    // lenght format
    val = 16;
    os.write(reinterpret_cast<char*>(&val), 4);
    // type format
    h_val = 1;
    os.write(reinterpret_cast<char*>(&h_val), 2);
    // number of channels
    h_val = 2;
    os.write(reinterpret_cast<char*>(&h_val), 2);
    // sample rate
    val = 44100;
    os.write(reinterpret_cast<char*>(&val), 4);
    // (Sample Rate * BitsPerSample * Channels) / 8
    val = 176400;
    os.write(reinterpret_cast<char*>(&val), 4);
    // (BitsPerSample * Channels) / 8.1
    h_val = 4;
    os.write(reinterpret_cast<char*>(&h_val), 2);
    // bits per sample
    h_val = 16;
    os.write(reinterpret_cast<char*>(&h_val), 2);
    // data chunk header
    os.write("data", 4);
    // data size
    if(samples_L.size()!=samples_R.size())
        exit(6);
    val = samples_L.size()*2*2;
    os.write(reinterpret_cast<char*>(&val), 4);

    for(auto i : ranges::iota_view(0, int(samples_L.size()))){
        os.put(samples_L[i]&0xFF);
        os.put((samples_L[i]>>8)&0xFF);
        os.put(samples_R[i]&0xFF);
        os.put((samples_R[i]>>8)&0xFF);
    }
    uint32_t pos = os.tellp() - 8;
    // cout << pos << endl;
    os.seekp(4, ios::beg);
    os.write(reinterpret_cast<char*>(&pos), 4);
}


int main(int argc, char* argv[])
{
    #ifdef DEBUG
    char* d_argv[] = {
                        (char*)"./qoa_decomp",
                        (char*)"./toms-diner.qoa",
                        (char*)"./output_file.wav",
                        nullptr,
                    };
    argv = d_argv;
    argc = 3;
    #endif

    if(argc != 3)
        exit(1);

    qoa_to_wav(argv[1], argv[2]);

    return 0;
}