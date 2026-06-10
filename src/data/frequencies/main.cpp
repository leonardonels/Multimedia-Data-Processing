#define _CRT_SECURE_NO_WARNINGS

#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <iterator>
#include <ranges>
/* ogni cosa che ha un begin() e un end() è un range */

void error(const std::string& message) {
	std::cout << message << '\n';
	exit(EXIT_FAILURE);
}

void syntax() {
	error("SYNTAX:\n"
		"frequences <inputfile> <outputfile>\n");
}

class myByte {
	uint8_t ch;
	size_t cnt = 1;
	
public:
	myByte(const uint8_t& c) {
		ch = c;
	}
	const uint8_t getCh() const {
		return ch;
	}
	const size_t getCnt() const {
		return cnt;
	}
	void increment() {
		cnt += 1;
	}
};

bool myByteCmp(const myByte& a, const myByte& b) {
	//  returns true if the first argument is less than (i.e. is ordered before) the second
	return a.getCh() < b.getCh();
}

int main(int argc, char* argv[]) {
	using namespace std;
	using namespace std::chrono;

	//auto start = steady_clock::now();

	if (argc != 3) {
		syntax();
	}

	// opening input file in binary mode
	std::ifstream is(argv[1], std::ios::binary);
	if (!is) {
		return EXIT_FAILURE;
	}
	// opening output file
	std::ofstream os(argv[2]);
	if (!os) {
		return EXIT_FAILURE;
	}

	// creo vettore di mybyte
	std::vector<myByte> v;

	auto start = steady_clock::now();

	// ciclo di lettura
	char byte;
	while (is.get(byte)) {
		bool found = 0;
		// cerco se ho già un mybyte con stesso valore
		/*
		auto it = find_if(v.begin(), v.end(), [byte](myByte& myByte) {
			if (myByte.getCh() == (uint8_t)byte) { 
				myByte.increment();
				return true;
			}
			else {
				return false;
			}
		});
		if (it == v.end()) {
			v.push_back(myByte((uint8_t)byte));
		}
		*/
		
		for (auto& myByte : v) {
			if (myByte.getCh() == (uint8_t)byte) {
				// l'ho trovato
				found = 1;
				myByte.increment();
				break;
			}
		}
		// se non ho trovato il mybyte corrispondente lo creo
		if (!found) {
			v.push_back(myByte((uint8_t)byte));
		}
		
	}

	auto stop = steady_clock::now();
	auto elapsed = stop - start;
	duration<double> elapsed_s = elapsed;
	std::cout << "Elapsed: " << elapsed_s << '\n';

	// ordino il vettore
	std::sort(v.begin(), v.end(), myByteCmp);

	// ciclo di scrittura
	for (auto& myByte : v) {
		os << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(myByte.getCh()) << '\t' 
			<< std::dec << myByte.getCnt() << '\n';
	}

	return EXIT_SUCCESS;
}