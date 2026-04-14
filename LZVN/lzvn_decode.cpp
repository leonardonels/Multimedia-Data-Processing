#include <string>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <vector>

const bool DEBUG = true;

using namespace std;

constexpr std::array<string_view, 256> opcode_table = {{/*00000000*/ "sml_d", /*00000001*/ "sml_d", /*00000010*/ "sml_d", /*00000011*/ "sml_d", /*00000100*/ "sml_d", /*00000101*/ "sml_d", /*00000110*/ "eos", /*00000111*/ "lrg_d", /*00001000*/ "sml_d", /*00001001*/ "sml_d", /*00001010*/ "sml_d", /*00001011*/ "sml_d", /*00001100*/ "sml_d", /*00001101*/ "sml_d", /*00001110*/ "nop", /*00001111*/ "lrg_d", /*00010000*/ "sml_d", /*00010001*/ "sml_d", /*00010010*/ "sml_d", /*00010011*/ "sml_d", /*00010100*/ "sml_d", /*00010101*/ "sml_d", /*00010110*/ "nop", /*00010111*/ "lrg_d", /*00011000*/ "sml_d", /*00011001*/ "sml_d", /*00011010*/ "sml_d", /*00011011*/ "sml_d", /*00011100*/ "sml_d", /*00011101*/ "sml_d", /*00011110*/ "udef", /*00011111*/ "lrg_d", /*00100000*/ "sml_d", /*00100001*/ "sml_d", /*00100010*/ "sml_d", /*00100011*/ "sml_d", /*00100100*/ "sml_d", /*00100101*/ "sml_d", /*00100110*/ "udef", /*00100111*/ "lrg_d", /*00101000*/ "sml_d", /*00101001*/ "sml_d", /*00101010*/ "sml_d", /*00101011*/ "sml_d", /*00101100*/ "sml_d", /*00101101*/ "sml_d", /*00101110*/ "udef", /*00101111*/ "lrg_d", /*00110000*/ "sml_d", /*00110001*/ "sml_d", /*00110010*/ "sml_d", /*00110011*/ "sml_d", /*00110100*/ "sml_d", /*00110101*/ "sml_d", /*00110110*/ "udef", /*00110111*/ "lrg_d", /*00111000*/ "sml_d", /*00111001*/ "sml_d", /*00111010*/ "sml_d", /*00111011*/ "sml_d", /*00111100*/ "sml_d", /*00111101*/ "sml_d", /*00111110*/ "udef", /*00111111*/ "lrg_d", /*01000000*/ "sml_d", /*01000001*/ "sml_d", /*01000010*/ "sml_d", /*01000011*/ "sml_d", /*01000100*/ "sml_d", /*01000101*/ "sml_d", /*01000110*/ "pre_d", /*01000111*/ "lrg_d", /*01001000*/ "sml_d", /*01001001*/ "sml_d", /*01001010*/ "sml_d", /*01001011*/ "sml_d", /*01001100*/ "sml_d", /*01001101*/ "sml_d", /*01001110*/ "pre_d", /*01001111*/ "lrg_d", /*01010000*/ "sml_d", /*01010001*/ "sml_d", /*01010010*/ "sml_d", /*01010011*/ "sml_d", /*01010100*/ "sml_d", /*01010101*/ "sml_d", /*01010110*/ "pre_d", /*01010111*/ "lrg_d", /*01011000*/ "sml_d", /*01011001*/ "sml_d", /*01011010*/ "sml_d", /*01011011*/ "sml_d", /*01011100*/ "sml_d", /*01011101*/ "sml_d", /*01011110*/ "pre_d", /*01011111*/ "lrg_d", /*01100000*/ "sml_d", /*01100001*/ "sml_d", /*01100010*/ "sml_d", /*01100011*/ "sml_d", /*01100100*/ "sml_d", /*01100101*/ "sml_d", /*01100110*/ "pre_d", /*01100111*/ "lrg_d", /*01101000*/ "sml_d", /*01101001*/ "sml_d", /*01101010*/ "sml_d", /*01101011*/ "sml_d", /*01101100*/ "sml_d", /*01101101*/ "sml_d", /*01101110*/ "pre_d", /*01101111*/ "lrg_d", /*01110000*/ "udef", /*01110001*/ "udef", /*01110010*/ "udef", /*01110011*/ "udef", /*01110100*/ "udef", /*01110101*/ "udef", /*01110110*/ "udef", /*01110111*/ "udef", /*01111000*/ "udef", /*01111001*/ "udef", /*01111010*/ "udef", /*01111011*/ "udef", /*01111100*/ "udef", /*01111101*/ "udef", /*01111110*/ "udef", /*01111111*/ "udef", /*10000000*/ "sml_d", /*10000001*/ "sml_d", /*10000010*/ "sml_d", /*10000011*/ "sml_d", /*10000100*/ "sml_d", /*10000101*/ "sml_d", /*10000110*/ "pre_d", /*10000111*/ "lrg_d", /*10001000*/ "sml_d", /*10001001*/ "sml_d", /*10001010*/ "sml_d", /*10001011*/ "sml_d", /*10001100*/ "sml_d", /*10001101*/ "sml_d", /*10001110*/ "pre_d", /*10001111*/ "lrg_d", /*10010000*/ "sml_d", /*10010001*/ "sml_d", /*10010010*/ "sml_d", /*10010011*/ "sml_d", /*10010100*/ "sml_d", /*10010101*/ "sml_d", /*10010110*/ "pre_d", /*10010111*/ "lrg_d", /*10011000*/ "sml_d", /*10011001*/ "sml_d", /*10011010*/ "sml_d", /*10011011*/ "sml_d", /*10011100*/ "sml_d", /*10011101*/ "sml_d", /*10011110*/ "pre_d", /*10011111*/ "lrg_d", /*10100000*/ "med_d", /*10100001*/ "med_d", /*10100010*/ "med_d", /*10100011*/ "med_d", /*10100100*/ "med_d", /*10100101*/ "med_d", /*10100110*/ "med_d", /*10100111*/ "med_d", /*10101000*/ "med_d", /*10101001*/ "med_d", /*10101010*/ "med_d", /*10101011*/ "med_d", /*10101100*/ "med_d", /*10101101*/ "med_d", /*10101110*/ "med_d", /*10101111*/ "med_d", /*10110000*/ "med_d", /*10110001*/ "med_d", /*10110010*/ "med_d", /*10110011*/ "med_d", /*10110100*/ "med_d", /*10110101*/ "med_d", /*10110110*/ "med_d", /*10110111*/ "med_d", /*10111000*/ "med_d", /*10111001*/ "med_d", /*10111010*/ "med_d", /*10111011*/ "med_d", /*10111100*/ "med_d", /*10111101*/ "med_d", /*10111110*/ "med_d", /*10111111*/ "med_d", /*11000000*/ "sml_d", /*11000001*/ "sml_d", /*11000010*/ "sml_d", /*11000011*/ "sml_d", /*11000100*/ "sml_d", /*11000101*/ "sml_d", /*11000110*/ "pre_d", /*11000111*/ "lrg_d", /*11001000*/ "sml_d", /*11001001*/ "sml_d", /*11001010*/ "sml_d", /*11001011*/ "sml_d", /*11001100*/ "sml_d", /*11001101*/ "sml_d", /*11001110*/ "pre_d", /*11001111*/ "lrg_d", /*11010000*/ "udef", /*11010001*/ "udef", /*11010010*/ "udef", /*11010011*/ "udef", /*11010100*/ "udef", /*11010101*/ "udef", /*11010110*/ "udef", /*11010111*/ "udef", /*11011000*/ "udef", /*11011001*/ "udef", /*11011010*/ "udef", /*11011011*/ "udef", /*11011100*/ "udef", /*11011101*/ "udef", /*11011110*/ "udef", /*11011111*/ "udef", /*11100000*/ "lrg_l", /*11100001*/ "sml_l", /*11100010*/ "sml_l", /*11100011*/ "sml_l", /*11100100*/ "sml_l", /*11100101*/ "sml_l", /*11100110*/ "sml_l", /*11100111*/ "sml_l", /*11101000*/ "sml_l", /*11101001*/ "sml_l", /*11101010*/ "sml_l", /*11101011*/ "sml_l", /*11101100*/ "sml_l", /*11101101*/ "sml_l", /*11101110*/ "sml_l", /*11101111*/ "sml_l", /*11110000*/ "lrg_m", /*11110001*/ "sml_m", /*11110010*/ "sml_m", /*11110011*/ "sml_m", /*11110100*/ "sml_m", /*11110101*/ "sml_m", /*11110110*/ "sml_m", /*11110111*/ "sml_m", /*11111000*/ "sml_m", /*11111001*/ "sml_m", /*11111010*/ "sml_m", /*11111011*/ "sml_m", /*11111100*/ "sml_m", /*11111101*/ "sml_m", /*11111110*/ "sml_m", /*11111111*/ "sml_m"}};

class lzvn_decode
{
private:
    ifstream &is_;
    ofstream &os_;

    uint32_t LVZN = 0x6e787662;        // bvxn
    uint32_t ENDOFSTREAM = 0x24787662; // bvx$

    uint32_t buffer_;
    vector<uint8_t> out_buffer_;
    uint32_t dist_;

public:
    size_t decoded_size_;
    size_t block_size_;

    lzvn_decode(ifstream &is, ofstream &os) : is_(is), os_(os) {}
    ~lzvn_decode() = default;

    uint32_t read_buffer(size_t size = 4)
    {
        if (size > 4)
            exit(104);
        buffer_ = 0;
        is_.read(reinterpret_cast<char *>(&buffer_), size);
        return buffer_;
    }

    string_view read_opcode()
    {
        return opcode_table[read_buffer(1) & 0xFF];
    }

    string_view get_opcode()
    {
        return opcode_table[buffer_];
    }

    size_t get_size()
    {
        return out_buffer_.size();
    }

    bool push_l(uint8_t len)
    {
        for (size_t i = 0; i < len; i++)
        {
            out_buffer_.push_back(read_buffer(1) & 0xFF);
        }
        return true;
    }

    bool push_m(uint16_t match_len)
    {
        size_t pos = out_buffer_.size();
        for (size_t i = 0; i < match_len; i++)
        {
            if ((pos - dist_ + i) >= out_buffer_.size())
                return false;
            out_buffer_.push_back(out_buffer_[pos - dist_ + i]);
        }
        return true;
    }

    bool read_header()
    {
        if (read_buffer() != LVZN)
            return false;
        decoded_size_ = read_buffer();
        block_size_ = read_buffer();
        return true;
    }

    bool sml_l()
    {
        return push_l(buffer_ & 0xF);
    }

    bool lrg_l()
    {
        return push_l((uint8_t)(read_buffer(1) & 0xFF) + 16);
    }

    bool pre_d()
    {
        uint16_t match_len = ((uint8_t)((buffer_ >> 3) & 0x7)) + 3;
        return push_l((buffer_ >> 6) & 0x3) && push_m(match_len);
    }

    bool sml_m()
    {
        return push_m((uint8_t)(buffer_ & 0xF));
    }

    bool lrg_m()
    {
        return push_m(((uint8_t)read_buffer(1) & 0xFF) + 16);
    }

    bool sml_d()
    {
        uint8_t len = (buffer_ >> 6) & 0x3;
        uint16_t match_len = ((uint8_t)((buffer_ >> 3) & 0x7));

        dist_ = buffer_ & 0x7;
        dist_ = (dist_ << 8) + (uint8_t)(read_buffer(1) & 0xFF);

        return push_l(len) && push_m(match_len + 3);
    }

    bool med_d()
    {
        uint8_t len = (buffer_ >> 3) & 0x3;

        uint16_t match_len = ((uint8_t)(buffer_ & 0x7));
        match_len = (match_len << 2) + (uint8_t)(read_buffer(1) & 0x3);

        dist_ = (uint8_t)((buffer_ >> 2) & 0x3F);
        dist_ = dist_ + ((read_buffer(1) & 0xFF) << 6);

        return push_l(len) && push_m(match_len + 3);
    }

    bool lrg_d()
    {
        uint8_t len = (buffer_ >> 6) & 0x3;
        uint16_t match_len = ((uint8_t)((buffer_ >> 3) & 0x7));
        dist_ = (uint16_t)(read_buffer(2) & 0xFFFF);

        return push_l(len) && push_m(match_len + 3);
    }

    bool decode_stream()
    {
        if (read_opcode() == "nop")
            return true;

        if (get_opcode() == "eos")
            return false;
        if (get_opcode() == "sml_l")
            return sml_l();
        if (get_opcode() == "lrg_l")
            return lrg_l();
        if (get_opcode() == "pre_d")
            return pre_d();
        if (get_opcode() == "sml_m")
            return sml_m();
        if (get_opcode() == "lrg_m")
            return lrg_m();
        if (get_opcode() == "sml_d")
            return sml_d();
        if (get_opcode() == "med_d")
            return med_d();
        if (get_opcode() == "lrg_d")
            return lrg_d();

        return false; // last case should be udef and it should never happen
    }

    void write()
    {
        for (size_t i = 0; i < out_buffer_.size(); i++)
        {
            os_.put(out_buffer_[i]);
        }
    }
};

int main(int argc, char *argv[])
{
    if (argc != 3 && !DEBUG)
        exit(1);
    string input_file, output_file;

    if (DEBUG)
    {
        input_file = "/home/surface/Downloads/output02.lzvn";
        output_file = "/home/surface/Downloads/outputlzvn02.txt";
    }
    else
    {
        input_file = argv[1], output_file = argv[2];
    }

    ifstream is(input_file, ios::binary);
    if (!is)
        exit(2);

    ofstream os(output_file, ios::binary);
    if (!os)
        exit(3);

    lzvn_decode lz(is, os);

    if (!lz.read_header())
        exit(4);

    bool stream = true;
    do
    {
        stream = lz.decode_stream();
    } while ((lz.get_size() <= lz.decoded_size_) && stream);

    lz.write();

    return 0;
}
