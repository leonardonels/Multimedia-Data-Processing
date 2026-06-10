#ifndef Y4M_COLOR_CPP
#define Y4M_COLOR_CPP

#define _CRT_SECURE_NO_WARNINGS 

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <exception>
#include "mat.h"
#include "types.h"
#include "utils.h"

void error(const std::string& message) {
	std::cout << message << std::endl;
	exit(EXIT_FAILURE);
}
void check_open(const std::ifstream& stream, const std::string& filename) {
	if (!stream) { error("Impossible read input file: " + filename); }
}

std::tuple<int, int> read_header(std::ifstream& is) {
	// read YUV4MPEG2
	std::string buffer;
	is >> buffer;
	if (buffer != "YUV4MPEG2") { error("Wrong header"); }
	// read other params, the only important are W and H
	int W = 0;
	int H = 0;
	bool finished = false;
	while (!finished) {
		is >> buffer;
		if (buffer.data()[0] == 'W') {
			buffer.erase(0, 1);
			try {
				W = std::stoi(buffer);
			}
			catch (const std::exception&) {
				error("Wrong header");
			}
		}
		else if (buffer.data()[0] == 'H') {
			buffer.erase(0, 1);
			try {
				H = std::stoi(buffer);
			}
			catch (const std::exception&) {
				error("Wrong header");
			}
		}
		else if (buffer == "FRAME") {
			is.seekg(-5, std::ios::cur);
			finished = true;
		}
	}
	// if W or H not read, an error is occured
	if (W * H == 0) {
		error("Wrong header");
	}
	// return W and H
	return std::tuple<int, int>(W, H);
}

bool read_frame_header(std::ifstream& is) {
	std::string buffer;
	is >> buffer;
	if (is.eof()) { return false; }
	if (buffer != "FRAME") { error("Wrong frame header"); }
	while (is.peek() == 'I' || is.peek() == 'X') { is >> buffer; }
	return true;
}

void satYCbCr(std::vector<uint8_t>& ys, std::vector<uint8_t>& cbs, std::vector<uint8_t>& crs, size_t frame_size) {
	for (size_t i = 0; i < frame_size; ++i) {
		// Y values between 16 and 235
		if (ys[i] < 16) { ys[i] = 16; }
		else if (ys[i] > 235) { ys[i] = 235; }

		// Cb and Cr values between 16 and 240
		if (cbs[i] < 16) { cbs[i] = 16; }
		else if (cbs[i] > 240) { cbs[i] = 240; }

		if (crs[i] < 16) { crs[i] = 16; }
		else if (crs[i] > 240) { crs[i] = 240; }
	}
}
uint8_t satRGB(int x) {
	if (x < 0) { return 0; }
	else if (x > 255) { return 255; }
	return x;
}

void getCbs_or_Crs(std::ifstream& is, std::vector<uint8_t>& cs, int rows, int cols) {
	uint8_t buffer;

	for (int i = 0; i < rows/2; ++i) {
		for (int j = 0; j < cols/2; ++j) {
			is.read(reinterpret_cast<char*>(&buffer), 1);
			cs[2 * i * cols + 2 * j] = buffer;
			cs[2 * i * cols + 2 * j + 1] = buffer;
			cs[(2 * i + 1) * cols + 2 * j] = buffer;
			cs[(2 * i + 1) * cols + 2 * j + 1] = buffer;
		}
	}
}

void read_frame(mat<vec3b>& frame, std::ifstream& is) {
	is.ignore(1);
	// read W*H [=size] uint8_t for Y, (W/2)*(W/2) [=size/4] uint8_t for Cb, (W/2)*(W/2) [=size/4] uint8_t for Cr
	// total lenght = size + size/4 + size/4 = 3/2 size
	size_t frame_size = frame.size();
	int rows = frame.rows();
	int cols = frame.cols();
	std::vector<uint8_t> ys(frame_size);
	std::vector<uint8_t> cbs(frame_size);
	std::vector<uint8_t> crs(frame_size);

	is.read(reinterpret_cast<char*>(&ys[0]), frame_size);
	getCbs_or_Crs(is, cbs, rows, cols);
	getCbs_or_Crs(is, crs, rows, cols);

	// saturate YCbCr values
	satYCbCr(ys, cbs, crs, frame_size);
	// first size bytes of bytes are Y, from size+1 to 5/4*size are Cb, from (5/4+1)size to 3/2*size are Cr
	size_t i = 0;
	for (vec3b& pixel : frame) {
		// for each pixel save RGB values
		// RED		R = 1.164*(Y-16)				  + 1.596*(Cr-128)
		pixel[0] = satRGB(1.164 * (ys[i] - 16) + 1.596 * (crs[i] - 128));
		// GREEN	G = 1.164*(Y-16) - 0.392*(Cb-128) - 0.813*(Cr-128)
		pixel[1] = satRGB(1.164 * (ys[i] - 16) - 0.392 * (cbs[i] - 128) - 0.813 * (crs[i] - 128));
		// BLUE		B = 1.164*(Y-16) + 2.017*(Cb-128)
		pixel[2] = satRGB(1.164 * (ys[i] - 16) + 2.017 * (cbs[i] - 128));
		++i;
	}
}

bool read_frames(std::ifstream& is, int W, int H, std::vector<mat<vec3b>>& frames) {
	// read frame header
	while (read_frame_header(is)) {
		mat<vec3b> frame(H, W);
		// read all bytes
		try { read_frame(frame, is); }
		catch (const std::exception&) { return false; }
		frames.push_back(frame);
	}
	return true;
}

bool y4m_extract_color(const std::string& filename, std::vector<mat<vec3b>>& frames) {
	std::ifstream is(filename, std::ios::binary);
	check_open(is, filename);

	// read header
	const auto [W, H] = read_header(is);
	std::cout << "Header read. W=" << W << ", H=" << H << std::endl;

	// read frames
	bool result = read_frames(is, W, H, frames);

	return result;
}


#endif // !Y4M_COLOR_CPP