#include <algorithm>
#include <bit>
#include <fstream>
#include <iterator>
#include <iostream>
#include <string>
#include <vector>

void error(const std::string& message) {
	std::cout << message << '\n';
	exit(EXIT_FAILURE);
}

void syntax() {
	error(
		"SYNTAX:\n"
		"elias [c|d] <filein> <fileout>\n"
	);
}

class bitreader {
	uint8_t buffer_ = 0;
	size_t n_ = 0; // 0-8
	std::istream& is_;

	uint64_t readbit() {
		if (n_ == 0) {
			buffer_ = is_.get();
			n_ = 8;
		}
		--n_;
		return (buffer_ >> n_) & 1;
	}

public:
	bitreader(std::istream& is) : is_(is) {}

	uint64_t operator()(uint64_t numbits) {
		uint64_t u = 0;
		while (numbits --> 0) {
			u = (u << 1) | readbit();
		}
		return u;
	}

	bool fail() const {
		return is_.fail();
	}

	operator bool() const {
		return !fail();
	}
};

class bitwriter {
	uint8_t buffer_ = 0;
	size_t n_ = 0; // 0-8
	std::ostream& os_;

	void writebit(uint64_t curbit) {
		buffer_ = (buffer_ << 1) | (curbit & 1);
		++n_;
		if (n_ == 8) {
			os_.put(buffer_);
			n_ = 0;
		}
	}

public:
	bitwriter(std::ostream& os) : os_(os) {}

	~bitwriter() {
		flush();
	}

	std::ostream& operator()(uint64_t x, uint64_t numbits) {
		while (numbits --> 0) {
			writebit(x >> numbits);
		}
		return os_;
	}

	std::ostream& flush(int padbit = 0) {
		while (n_ > 0) {
			writebit(padbit);
		}
		return os_;
	}
};

class elias_reader {
	bitreader br_;
	static int unmap(uint64_t x) {
		if (x % 2 == 0) {	// pari
			return -static_cast<int>(x / 2);
		}	// dispari
		return static_cast<int>((x - 1) / 2);
	}
public:
	elias_reader(std::istream& is) : br_(is) {}
	int read() {
		uint64_t n = 0;
		// tolgo gli zeri iniziali
		while (br_(1) == 0) {
			++n;
		}
		uint64_t val = (1LL << n) | br_(n);
		return unmap(val);
	}
};

class elias_writer {
	bitwriter bw_;
	static uint64_t map(int x) {
		if (x < 0) {	// negativo
			return -x * 2;
		}	// positivo
		return x * 2 + 1;
	}
	static uint64_t numbits(uint64_t x) {
		auto bw = std::bit_width(x);
		return bw * 2 - 1;
	}
public:
	elias_writer(std::ostream& os) : bw_(os) {}
	void write(int x) {
		uint64_t mapped = map(x);
		bw_(mapped, numbits(mapped));
	}
};

void compress(const std::string& input_filename, const std::string& output_filename) {
	std::ifstream is(input_filename/*, std::ios::binary*/);
	if (!is) {
		error("Cannot open file " + input_filename + " for reading");
	}
	std::ofstream os(output_filename, std::ios::binary);
	if (!os) {
		error("Cannot open file " + output_filename + " for writing");
	}
	elias_writer ew(os);

	std::for_each(std::istream_iterator<int>(is), std::istream_iterator<int>(), [&](int num) { ew.write(num); });
}

void decompress(const std::string& input_filename, const std::string& output_filename) {
	std::ifstream is(input_filename, std::ios::binary);
	if (!is) {
		error("Cannot open file " + input_filename + " for reading");
	}
	std::ofstream os(output_filename/*, std::ios::binary*/);
	if (!os) {
		error("Cannot open file " + output_filename + " for writing");
	}
	elias_reader er(is);

	while (true) {
		int num = er.read();
		if (!is) { break; }
		os << num << "\n";
	}
}

int main(int argc, char* argv[]) {
	if (argc != 4) { syntax(); }

	std::string command = argv[1];
	if (command == "c") { compress(argv[2], argv[3]); }
	else if (command == "d") { decompress(argv[2], argv[3]); }
	else { syntax(); }

	return EXIT_SUCCESS;
}