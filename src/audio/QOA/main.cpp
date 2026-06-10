#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <cstdint>
#include <array>
#include <algorithm>

// Constants
constexpr int WAV_HEADER_SIZE = 44;
constexpr int BITS_PER_SAMPLE = 16;
constexpr int CHANNELS = 2;
constexpr int SAMPLE_RATE = 44100;
constexpr int BLOCK_ALIGN = (BITS_PER_SAMPLE / 8) * CHANNELS;
constexpr int BYTE_RATE = SAMPLE_RATE * BLOCK_ALIGN;
constexpr int SLICE_SAMPLES = 20;
constexpr int SLICES_PER_FRAME = 256;

struct QOAFileHeader {
    char magic[4];     // "qoaf"
    uint32_t samples;  // Total samples per channel (big-endian)
};

struct QOAFrameHeader {
    uint8_t num_channels;
    uint32_t samplerate; // Big-endian uint24_t
    uint16_t fsamples;   // Big-endian
    uint16_t fsize;      // Big-endian
};

struct LMSState {
    std::array<int16_t, 4> history;
    std::array<int16_t, 4> weights;
};

// Helper function to convert big-endian to little-endian
uint32_t big_to_little_endian(uint32_t value) {
    return ((value & 0xFF000000) >> 24) |
        ((value & 0x00FF0000) >> 8) |
        ((value & 0x0000FF00) << 8) |
        ((value & 0x000000FF) << 24);
}

uint16_t big_to_little_endian(uint16_t value) {
    return ((value & 0xFF00) >> 8) | ((value & 0x00FF) << 8);
}

// Function to write WAV header
void write_wav_header(std::ofstream& out, uint32_t total_samples) {
    uint32_t data_size = total_samples * BLOCK_ALIGN;
    uint32_t file_size = data_size + WAV_HEADER_SIZE - 8;

    out.write("RIFF", 4);
    uint32_t file_size_le = big_to_little_endian(file_size);
    out.write(reinterpret_cast<const char*>(&file_size_le), 4);
    out.write("WAVE", 4);
    out.write("fmt ", 4);

    uint32_t fmt_chunk_size = 16;
    uint16_t audio_format = 1; // PCM

    out.write(reinterpret_cast<const char*>(&fmt_chunk_size), 4);
    out.write(reinterpret_cast<const char*>(&audio_format), 2);
    out.write(reinterpret_cast<const char*>(&CHANNELS), 2);
    out.write(reinterpret_cast<const char*>(&SAMPLE_RATE), 4);
    out.write(reinterpret_cast<const char*>(&BYTE_RATE), 4);
    out.write(reinterpret_cast<const char*>(&BLOCK_ALIGN), 2);
    out.write(reinterpret_cast<const char*>(&BITS_PER_SAMPLE), 2);

    out.write("data", 4);
    uint32_t data_size_le = big_to_little_endian(data_size);
    out.write(reinterpret_cast<const char*>(&data_size_le), 4);
}

// Function to decode a single slice
void decode_slice(const uint8_t* slice, LMSState& lms_state, std::vector<int16_t>& output) {
    uint8_t sf_quant = slice[0];
    int32_t sf = static_cast<int32_t>(std::round(std::pow(sf_quant + 1, 2.75)));

    for (int i = 0; i < SLICE_SAMPLES; ++i) {
        uint8_t qr = (slice[1 + i / 2] >> (4 * (i % 2))) & 0xF;
        static const float dequant_tab[] = { 0.75, -0.75, 2.5, -2.5, 4.5, -4.5, 7, -7 };

        int32_t r = sf * dequant_tab[qr];
        r = (r < 0) ? static_cast<int32_t>(std::ceil(r - 0.5)) : static_cast<int32_t>(std::floor(r + 0.5));

        int32_t p = 0;
        for (int n = 0; n < 4; ++n) {
            p += lms_state.history[n] * lms_state.weights[n];
        }
        p >>= 13;

        int32_t s = std::clamp(p + r, -32768, 32767);
        output.push_back(static_cast<int16_t>(s));

        int32_t delta = r >> 4;
        for (int n = 0; n < 4; ++n) {
            lms_state.weights[n] += (lms_state.history[n] < 0) ? -delta : delta;
        }

        for (int n = 0; n < 3; ++n) {
            lms_state.history[n] = lms_state.history[n + 1];
        }
        lms_state.history[3] = static_cast<int16_t>(s);
    }
}

void decode_qoa(const std::string& input_file, const std::string& output_file) {
    std::ifstream qoa_in(input_file, std::ios::binary);
    std::ofstream wav_out(output_file, std::ios::binary);

    if (!qoa_in || !wav_out) {
        std::cerr << "Error opening files." << std::endl;
        return;
    }

    QOAFileHeader qoa_header;
    qoa_in.read(reinterpret_cast<char*>(&qoa_header), sizeof(QOAFileHeader));
    if (std::string(qoa_header.magic, 4) != "qoaf") {
        std::cerr << "Invalid QOA file." << std::endl;
        return;
    }

    uint32_t total_samples = big_to_little_endian(qoa_header.samples) * CHANNELS;
    write_wav_header(wav_out, total_samples);

    LMSState lms_states[CHANNELS] = {};
    std::vector<int16_t> output_samples;

    while (qoa_in) {
        QOAFrameHeader frame_header;
        qoa_in.read(reinterpret_cast<char*>(&frame_header), sizeof(QOAFrameHeader));
        if (!qoa_in) break;

        frame_header.fsamples = big_to_little_endian(frame_header.fsamples);

        for (int c = 0; c < CHANNELS; ++c) {
            qoa_in.read(reinterpret_cast<char*>(&lms_states[c]), sizeof(LMSState));
        }

        for (int s = 0; s < SLICES_PER_FRAME; ++s) {
            for (int c = 0; c < CHANNELS; ++c) {
                uint8_t slice[8];
                qoa_in.read(reinterpret_cast<char*>(slice), sizeof(slice));
                if (!qoa_in) break;

                decode_slice(slice, lms_states[c], output_samples);
            }
        }
    }

    wav_out.write(reinterpret_cast<const char*>(output_samples.data()), output_samples.size() * sizeof(int16_t));

    std::cout << "Decompression complete." << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: qoa_decomp <input file> <output file>" << std::endl;
        return 1;
    }

    decode_qoa(argv[1], argv[2]);
    return 0;
}
