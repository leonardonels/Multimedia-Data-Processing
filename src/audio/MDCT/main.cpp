#include <fstream>
#include <iostream>
#include <cstdint>
#include <unordered_map>
#include <ranges>
#include <vector>
#include <cmath>

using namespace std;

int readFrom(ifstream& is)
{
    int x, y;
    x = is.get();
    if(x == -1)
        exit(101);
    y = is.get();
    if(y == -1)
        exit(102);

    int16_t buffer = (uint8_t)x | ((uint8_t)y << 8);
    return (int)buffer;
}

int main()
{
    ifstream is("./test.raw", ios::binary);
    if(!is)
        exit(1);

    is.seekg(0, ios::end);
    streamsize size = is.tellg() / 2;
    is.seekg(0, ios::beg);

    cout << "The file contains " << size << " 16 bit samples.\n";
    // ---

    unordered_map<int, size_t> umap;
    for(size_t i : ranges::iota_view(0, (int)size)){
        umap[readFrom(is)]++;
    }
    is.clear();
    is.seekg(0, ios::beg);

    double H = 0.0;
    for(auto [value, count] : umap){
        double x = ((double)(count) / size);  // n_i / N
        H -= x * log2(x);
    }
    cout << "Entropy of the raw original file is: " << H << endl;
    // ---

    unordered_map<int, size_t> q_map;
    vector<int> q;
    vector<int> og;
    int q_factor = 2600;
    for(size_t i : ranges::iota_view(0, (int)size)){
        int x = readFrom(is);
        og.push_back(x);
        q.push_back(x/q_factor);
        q_map[x/q_factor]++;
    }
    is.clear();
    is.seekg(0, ios::beg);

    double qH = 0.0;
    for(auto [value, count] : q_map){
        double x = ((double)(count) / size);  // n_i / N
        qH -= x * log2(x);
    }
    cout << "Entropy of the quantized signal is: " << qH << endl;

    ofstream qos("./output_qt.raw", ios::binary);
    if(!qos)
        exit(2);

    // Rebuild the data by de-quantizing and save it
    for(size_t i : ranges::iota_view(0, (int)size)){
        int x = q[i] * q_factor;
        qos.put((x)&0xFF);
        qos.put((x>>8)&0xFF);
    }
    // ---
    
    ofstream eos("./error_qt.raw", ios::binary);
    if(!eos)
        exit(3);

    for(size_t i : ranges::iota_view(0, (int)size)){
        int16_t x = og[i] - (q[i] * q_factor);
        eos.put((x)&0xFF);
        eos.put((x>>8)&0xFF);
    }
    // ---

    vector<int32_t> coef;
    size_t N = 1024;    // window
    int32_t Q = 10000;  // quantize factor
    vector<double> w_vec;
    vector<double> cos_vec;
    for(size_t k = 0; k < N; k++){
        for(size_t n = 0; n < 2*N; n++){
            if(k == 0)
                w_vec.push_back(sin((M_PI/(2*N))*(n+0.5)));
            cos_vec.push_back(cos((M_PI/N)*(n + 0.5 + N/2.0)*(k + 0.5)));
        }
    }
    size_t pad = ceil((double)size/N)*N - size;
    size_t L = size + pad;
    size_t blocks = L/N + 2;
    for(auto i : ranges::iota_view(0, int(blocks - 1))){
        for(size_t k = 0; k < N; k++){
            double sum = 0;
            for(size_t n = 0; n < 2*N; n++){
                
                double x;
                long long idx = (long long)i*N + n - N;
                if (idx < 0 || idx >= (long long)og.size())
                    x = 0.0;
                else
                    x = (double)og[idx];

                double w = w_vec[n];
                double c = cos_vec[k*2*N + n];
                sum += x*w*c;
                // cout << i << ", " << k << ", " << n << endl;
            }
            coef.push_back(static_cast<int32_t>(lround(sum/Q)));     // quantize
        }
    }

    unordered_map<int32_t, size_t> coef_map;
    for(auto x : coef){
        coef_map[x]++;
    }
    qH = 0.0;
    for(auto [_, x] : coef_map){
        double p = double(x) / coef.size();
        qH -= p * log2(p);
    }
    cout << "Entropy of the quantized coefficients is : " << qH << endl;

    // ---
    size_t r_blocks = coef.size()/N;
    vector<double> out((r_blocks*N)+N, 0.0);
    int NN = 2*N;
    for(auto i : ranges::iota_view(0, int(r_blocks))){
        for(size_t n : ranges::iota_view(0, NN)){
            double y_n = 0;
            for(size_t k : ranges::iota_view(0, int(N))){
                y_n += (double)coef[i*N+k] * Q * cos_vec[k*2*N + n];
            }
            y_n *= 2.0/N*w_vec[n];
            out[i * N + n] += y_n;
        }
    }

    ofstream out_file("output.raw", ios::binary);
    if(!out_file)
        exit(4);

    for (size_t i = 0; i < (size_t)size; i++) {
        long rounded = lround(out[N + i]);
        rounded = clamp(rounded, -32768L, 32767L);
        int16_t s = static_cast<int16_t>(rounded);
        out_file.put(s & 0xff);
        out_file.put((s >> 8) & 0xff);
    }

    ofstream error_file("error.raw", ios::binary);
    if (!error_file)
        exit(5);
    
    for (size_t i = 0; i < (size_t)size; i++) {
        int32_t diff = (int32_t)og[i] - (int32_t)lround(out[N + i]);
        diff = clamp(diff, -32768, 32767);
        int16_t e = static_cast<int16_t>(diff);
        error_file.put(e & 0xff);
        error_file.put((e >> 8) & 0xff);
    }

    return 0;
}