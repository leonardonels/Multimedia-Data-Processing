#include <cstdlib>
#include <cstdint>
#include <string>
#include <iostream>
#include <fstream>
#include <exception>
#include <vector>
#include <algorithm>

void error(const std::string& message) {
	std::cout << message << std::endl;
	exit(EXIT_FAILURE);
}
template<typename T>
void check_open(T& stream, const std::string& filename) {
	if (!stream) { error("Unable to open file: " + filename); }
}
void wrong_header() {
	error("Wrong header");
}

template <typename T>
void read(std::ifstream& is, T& buffer, size_t n_bytes) {
	try {
		is.read(reinterpret_cast<char*>(&buffer), n_bytes);
	}
	catch (...) {
		error("Unable to read");
	}
}

struct dir_entry {
	uint16_t tag;
	uint16_t type;
	uint32_t count;
	uint32_t value;

	void operator()(std::ifstream& is) {
		read(is, tag, 2);
		read(is, type, 2);
		read(is, count, 4);
		read(is, value, 4);
	}
};
struct image {
	uint32_t width;
	uint32_t lenght;
	std::vector<uint8_t> data;

	image(uint32_t w, uint32_t l, std::ifstream& is) :width(w), lenght(l) {
		uint32_t s = size();
		data.reserve(s);
		data.resize(s);
		char c = is.peek();
		is.read(reinterpret_cast<char*>(&data[0]), s);
	}

	uint32_t size() {
		return width * lenght;
	}
	const uint32_t size() const {
		return width * lenght;
	}
};

image tiff_read(std::ifstream& is) {
	uint16_t buffer;
	// read II
	read(is, buffer, 2);
	if (buffer != 18761) { wrong_header(); }
	// read 42
	read(is, buffer, 2);
	if (buffer != 42) { wrong_header(); }

	uint32_t offset;
	read(is, offset, 4);
	is.seekg(offset, std::ios::beg);

	// IFD
	uint16_t n_entries;
	read(is, n_entries, 2);
	std::vector<dir_entry> entries(n_entries);
	for (auto& entry : entries) {
		entry(is);
	}

	auto it = std::find_if(entries.begin(), entries.end(), [&](const auto& entry) {return entry.tag == 256; });
	uint32_t width = it->value;

	it = std::find_if(entries.begin(), entries.end(), [&](const auto& entry) {return entry.tag == 257; });
	uint32_t lenght = it->value;

	it = std::find_if(entries.begin(), entries.end(), [&](const auto& entry) {return entry.tag == 273; });
	uint32_t img_start = it->value;
	is.seekg(img_start, std::ios::beg);

	// copy pixels
	image img(width, lenght, is);
	
	return img;
}

void pam_write(image& img, std::ofstream& os) {
	uint32_t w = img.width;
	uint32_t l = img.lenght;
	os << "P7" << std::endl <<
		"WIDTH " << w << std::endl <<
		"HEIGHT " << l << std::endl <<
		"DEPTH 1" << std::endl <<
		"MAXVAL 255" << std::endl <<
		"TUPLTYPE GRAYSCALE" << std::endl <<
		"ENDHDR" << std::endl;
	os.write(reinterpret_cast<const char*>(img.data.data()), img.data.size());
}

void tif2pam(std::ifstream& is, std::ofstream& os) {

	// read tif header and put image data into img
	image img = tiff_read(is);

	// write pam header and copy img
	pam_write(img, os);

	return;
}

int main(int argc, char** argv) {
	// controllo sui parametri
	if (argc != 3) { error("Wrong params number"); }

	std::string in_filename = argv[1];
	std::string out_filename = argv[2];

	std::ifstream is(in_filename, std::ios::binary);
	check_open(is, in_filename);
	std::ofstream os(out_filename, std::ios::binary);
	check_open(os, out_filename);

	tif2pam(is, os);

	return EXIT_SUCCESS;
}