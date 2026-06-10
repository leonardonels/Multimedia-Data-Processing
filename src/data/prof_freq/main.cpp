#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <array>
#include <chrono>
#include <format>
#include <vector>
#include <iterator>
#include <algorithm>
#include <ranges>

void error(const std::string& message) {
	std::cout << message << '\n';
	exit(EXIT_FAILURE);
}

void syntax() {
	error(
		"SYNTAX:\n"
		"frequencies <input file> <output file>\n"
	);
}

template<typename T>
void check_open(const T& stream, const std::string& filename) {
	if (!stream) {
		error(std::string("Cannot open file ") + filename);
	}
}

struct freq {
	std::array<uint32_t, 256> data_ = {};

	void operator()(uint8_t val) {
		++data_[val];
	}
};

int main(int argc, char* argv[])
{
	using namespace std;
	using namespace std::chrono;
	auto start = steady_clock::now();

	if (argc != 3) {
		syntax();
	}

	std::ifstream is(argv[1], std::ios::binary);
	check_open(is, argv[1]);

	is.seekg(0, ios::end);
	auto filesize = is.tellg();
	is.seekg(0, ios::beg);
	vector<char> v(filesize);
	is.read(v.data(), filesize);

	//freq stats = for_each(begin(v), end(v), freq{});

	auto [a, stats] = ranges::for_each(v, freq{});

	std::ofstream os(argv[2]/*, std::ios::binary*/);
	check_open(os, argv[2]);

	os << std::setfill('0');
	for (size_t i = 0; i < 256; ++i) {
		if (stats.data_[i] > 0) {
			os << std::hex;
			os << std::setw(2) << i << '\t';
			os << std::dec;
			os << stats.data_[i] << '\n';
		}
	}

	auto stop = steady_clock::now();
	auto elapsed = stop - start;
	duration<double, milli> elapsed_s = elapsed;
	std::cout << "Elapsed: " << elapsed_s << "\n";

	return EXIT_SUCCESS;
}