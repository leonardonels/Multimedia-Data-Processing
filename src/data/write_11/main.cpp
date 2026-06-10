#include <iostream>
#include <fstream>
#include <vector>
//#include <iterator>

void error(const std::string& str) {
	std::cout << str << std::endl;
	exit(EXIT_FAILURE);
}

void syntax_error() {
	error("SYNTAX:\n"
	"write_int11 <filein.txt> <fileout.bin>\n");
}

void check_params(const int& argc) {
	if (argc != 3) {
		syntax_error();
	}
}

template<typename T>
void check_open(const T& stream, const std::string& filename) {
	if (!stream) {
		error(std::string("Impossible to open file: ") + filename);
	}
}

// BITWRITER CLASS
class bitwriter {
	uint8_t buffer_;
	size_t n_ = 0;
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

	std::ostream& operator()(uint64_t x, int numbits) {
		for (int bitnum = numbits - 1; bitnum >= 0; --bitnum) {
			writebit(x >> bitnum);
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

int main(int argc, char* argv[]) {

	// controllo parametri
	check_params(argc);

	// open input file in testual mode
	std::ifstream is(argv[1]/*, std::ios::binary*/);
	check_open(is, argv[1]);

	// read input data
	std::vector<int> v{ std::istream_iterator<int>(is), std::istream_iterator<int>()};

	// open output file in binary mode
	std::ofstream os(argv[2], std::ios::binary);
	check_open(os, argv[2]);

	// create bitwriter
	bitwriter bw(os);

	// write output data
	for (const auto& x : v) {
		bw(x, 11);
	}

	return EXIT_SUCCESS;
}