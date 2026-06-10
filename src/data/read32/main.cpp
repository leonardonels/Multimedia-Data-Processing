#include <array>
#include <vector>
#include <fstream>
#include <iostream>
#include <iterator>

void error(const std::string& message) {
	std::cout << message << '\n';
	exit(EXIT_FAILURE);
}

void syntax() {
	error(
		"SYNTAX:\n"
		"read_int32 <filein.bin> <fileout.txt>\n"
	);
}

template<typename T>
void check_open(const T& stream, const std::string& filename) {
	if (!stream) {
		error(std::string("Cannot open file ") + filename);
	}
}

int main(char argc, char* argv[]) {

	if (argc != 3) {
		syntax();
	}

	// creo input stream
	std::ifstream is(argv[1], std::ios::binary);
	check_open(is, argv[1]);

	std::vector<int> v;

	// ciclo lettura
	int num;
	while (is.read(reinterpret_cast<char*>(&num), sizeof(int))) {
		v.push_back(num);
	}
	
	// creo output stream
	std::ofstream os(argv[2]/*, std::ios::binary*/);
	check_open(os, argv[2]);

	// ciclo scrittura
	for (const auto& x : v) {
		os << x << std::endl;
	}

	return EXIT_SUCCESS;
}