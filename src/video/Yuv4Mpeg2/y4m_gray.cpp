#ifndef Y4M_GRAY_CPP
#define Y4M_GRAY_CPP

#define _CRT_SECURE_NO_WARNINGS 

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <exception>
#include "mat.h"
#include "types.h"

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
	if (buffer != "YUV4MPEG2") { error("Wrong header");	}
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

void read_frame(mat<uint8_t>& frame, std::ifstream& is) {
	is.ignore(1);
	// read W*H uint8_t for Y
	is.read(frame.rawdata(), frame.size());
	// skip 2* (W/2)*(H/2) [=W*H/2=size/2] uint8_t for Cb, Cr
	is.ignore(frame.size() / 2);
}

bool read_frames(std::ifstream& is, int W, int H, std::vector<mat<uint8_t>>& frames) {
	// read frame header
	while (read_frame_header(is)) {
		mat<uint8_t> frame(H, W);
		// read W*H uint8_t
		try { read_frame(frame, is); }
		catch (const std::exception&) {	return false; }
		frames.push_back(frame);
	}
	return true;
}

bool y4m_extract_gray(const std::string& filename, std::vector<mat<uint8_t>>& frames) {
	std::ifstream is(filename, std::ios::binary);
	check_open(is, filename);

	// read header
	const auto [W, H] = read_header(is);
	std::cout << "Header read. W=" << W << ", H=" << H << std::endl;

	// read frames
	bool result = read_frames(is, W, H, frames);
	
	return result;
}

#endif // !Y4M_GRAY_CPP