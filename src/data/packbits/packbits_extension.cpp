#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#define RUN 1
#define COPY 0

void error(const std::string& message) {
	std::cout << message << std::endl;
	exit(EXIT_FAILURE);
}
void syntax() {
	error("SYNTAX:\npackbits [c|d] <input file> <output file>");
}
template<typename T>
void check_open(const T& stream, const std::string& filename) {
	if (!stream) {
		error("ERROR:\ncannot open file " + filename);
	}
}

class packbits_encoder {
	char last_;
	char seclast_;
	size_t cnt_;
	bool mode_;
	std::vector<char> chars_;
	std::istream& is_;
	std::ostream& os_;

	void write_run(const size_t lenght){
		// aaaaa --> 252 a
		if (lenght < 2) { return; }
		char first = 257 - static_cast<char>(lenght);	// size of the run
		os_.put(first);		// 252, must be 129-255
		os_.put(last_);		// a, the symbol of the run
		chars_.clear();
	}
	void write_copy(const size_t lenght) {
		// cdefgh --> 5 c d e f g h 
		if (lenght < 1) { return; }
		char first = static_cast<char>(lenght - 1);
		os_.put(first);		// 5, must be 0-127
		for (const auto& c : chars_) {	// c d ... all the symbols
			os_.put(c);
		}
		chars_.clear();
	}
	void write_end() {
		char end_char = 128;
		os_.put(end_char);
	}
public:
	packbits_encoder(std::istream& is, std::ostream& os) : is_(is), os_(os) {
		last_ = 0;
		seclast_ = 0;
		cnt_ = 0;
		mode_ = COPY;
	}
	void operator()() {		// start reading and encoding
		// read first byte
		char read;
		is_.get(read);
		if (!read) {
			error("Empty input file");
		}
		chars_.push_back(read);
		last_ = read;
		cnt_ = 1;
		mode_ = COPY;

		while (is_.get(read)) {
			if (read == last_) {	// if new byte is equal to the last...
				if (mode_ == RUN) {	// ...and i am in RUN, i'll add that byte to the run
					if (cnt_ == 128) {	// then if the RUN is at limit i'll print and reset
						write_run(cnt_);
						chars_.push_back(read);
						seclast_ = 0;
						cnt_ = 1;
						mode_ = COPY;
					}
					else {
						++cnt_;
					}
				}
				else {	// (mode_ == COPY)		...and i am in COPY
					if (seclast_ == last_) {
						// it's the case like chars={a,b,c,d,f,f} read=f cnt=6 last=f
						// i need to pop 2 values, print the COPY and start a RUN of 3
						if (!chars_.empty()) { chars_.pop_back(); }
						if (!chars_.empty()) { chars_.pop_back(); }
						write_copy(cnt_ - 2);	// write the COPY, then update the status to RUN, last_ is the byte of the RUN
						cnt_ = 3;
						mode_ = RUN;
					}
					else {
						// it's the case like chars={a,b,c,d,e,f} read=f cnt=6 last=f
						chars_.push_back(read);
						seclast_ = last_;
						last_ = read;
						cnt_++;
						if (cnt_ == 2) {
							mode_ = RUN;
						}
					}
				}
			}
			else {	// (read != last_)	if new byte is not the last...
				if (mode_ == RUN) {		// ...and i am in RUN, i need to stop the RUN, write it, and start a COPY with the new byte
					write_run(cnt_);
					chars_.push_back(read);
					seclast_ = last_;
					last_ = read;
					cnt_ = 1;
					mode_ = COPY;	// and update the status
				}
				else {	// (mode_ == COPY)		...and i am in COPY, i'll add the byte to the vector
					if (cnt_ == 128) {
						write_copy(cnt_);
						cnt_ = 0;
					}
					chars_.push_back(read);
					seclast_ = last_;
					last_ = read;
					cnt_++;
				}
			}
		}
		if (mode_ == RUN) {
			write_run(cnt_);
		}
		else {
			write_copy(cnt_);
		}
		write_end();
	}
};

class packbits_decoder {
	std::istream& is_;
	std::ostream& os_;
public:
	packbits_decoder(std::istream& is, std::ostream& os):is_(is), os_(os){}
	std::ostream& operator()() {
		char byte;
		while (is_.get(byte)) {
			if (static_cast<uint8_t>(byte) == 128) {
				// end of file
				break;
			}
			else if (static_cast<uint8_t>(byte) < 128) {
				// COPY
				uint16_t n = static_cast<uint8_t>(byte) + 1;
				for (size_t i = 0; i < n; ++i) {
					is_.get(byte);
					if (!byte) {
						error("Invalid packbits format");
					}
					os_.put(byte);
				}
			}
			else { // byte > 128
				// RUN
				uint16_t n = 257 - static_cast<uint8_t>(byte);
				is_.get(byte);
				if (!byte) {
					error("Invalid packbits format");
				}
				for (size_t i = 0; i < n; ++i) {
					os_.put(byte);
				}
			}
		}
		return os_;
	}
};

int main(int argc, char* argv[]) {
	if (argc != 4) {
		syntax();
	}
	// open input stream in binary mode
	std::ifstream is(argv[2], std::ios::binary);
	check_open(is, argv[2]);
	// open output stream in binary mode
	std::ofstream os(argv[3], std::ios::binary);
	check_open(os, argv[3]);

	std::string command = argv[1];
	if (command == "c") {
		// compress mode
		packbits_encoder pe(is, os);
		pe();
	}
	else if (command == "d") {
		// decompress mode
		packbits_decoder pd(is, os);
		pd();
	}
	else {
		syntax();
	}
	return EXIT_SUCCESS;
}