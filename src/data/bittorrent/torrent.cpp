#include <cstdint>
#include <string>
#include <string_view>
#include <iostream>
#include <ostream>
#include <fstream>
#include <vector>	
#include <map>
#include <memory>
#include <iomanip>
#include <format>
#include <algorithm>

void error(const std::string& message) {
	std::cout << message << std::endl;
	exit(EXIT_FAILURE);
}
void syntax() {
	error("correct synthax:\ntorrent_dump <file .torrent>");
}
void format_error(const std::string& filename) {
	error(filename + " is not in correct format\n");
}
template<typename T>
void check_open(const T& stream, const std::string& filename) {
	if (!stream) {
		error("impossible to open file " + filename);
	}
}
char read_char_and_check(std::istream& is, const std::string& filename, const char& check) {
	char byte;
	is.get(byte);
	if (byte != check) { format_error(filename); }
	return byte;
}

namespace bncd {

	void newline(std::ostream& os, const size_t tabs, bool nl) {
		if(nl)	os << "\n";
		for (size_t i = 0; i < tabs; ++i) {
			os << "\t";
		}
	}
	void newline(std::ostream& os) {
		newline(os, 0, true);
	}
	void newline(std::ostream& os, const size_t tabs) {
		newline(os, tabs, true);
	}
	void skiptabs(std::ostream& os, const size_t tabs) {
		newline(os, tabs, false);
	}

	struct belement {
		virtual ~belement() = default;
		virtual void bprint(std::ostream& os, size_t tabs) const {}
		virtual void print_pieces(std::ostream& os, size_t tabs) const {}
		//virtual void bprint(std::ostream& os) const {}
	};

	using belement_ptr = std::unique_ptr<belement>;
	belement_ptr read(std::istream& is, const std::string& filename);  // Forward declaration

	// string
	struct bstring : belement {
		std::string str;
		int length;

		bstring(std::istream& is, const std::string& filename) {
			is >> length;
			char byte = read_char_and_check(is, filename, ':');
			// read string of lenght "lenght"
			for (size_t i = 0; i < length; ++i) {
				is.get(byte);
				if (byte < 32 || byte > 126) {
					str.push_back('.');
				}
				else {
					str.push_back(byte);
				}
			}
		}
		bstring(std::istream& is, const std::string& filename, bool isPieces) {
			is >> length;
			char byte = read_char_and_check(is, filename, ':');
			// read string of lenght "lenght"
			for (size_t i = 0; i < length; ++i) {
				is.get(byte);
				str.push_back(byte);
			}
		}
		void bprint(std::ostream& os, size_t tabs) const override {
			//skiptabs(os, tabs);
			os << "\"" << str << "\"";
			newline(os);
		}
		void bprint(std::ostream& os) const {
			os << "\"" << str << "\"";
		}
		void print_pieces(std::ostream& os, size_t tabs) const override {
			//skiptabs(os, tabs + 1);
			size_t cnt = 0;
			for (size_t i = 0; i < str.size(); ++i) {
				if (i % 20 == 0) {
					newline(os, tabs + 1);
				}
				std::cout << std::format("{:02x}", static_cast<unsigned char>(str[i]));
			}
			newline(os);
		}
		bool operator==(const std::string& rhs) {
			return str == rhs;
		}
		bool operator==(const bstring& rhs) {
			return str == rhs.str;
		}
	};

	using bstring_ptr = std::unique_ptr<bstring>;
	bstring_ptr read_pieces(std::istream& is, const std::string& filename);		// forward declaration

	// integers
	struct bint : public belement {
		int i;

		bint(std::istream& is, const std::string& filename) {
			read_char_and_check(is, filename, 'i');
			is >> i;
			read_char_and_check(is, filename, 'e');
		}
		void bprint(std::ostream& os, size_t tabs) const override {
			//skiptabs(os, tabs);
			os << i;
			newline(os);
		}
	};
	// list
	struct blist : public belement {
		std::vector<belement_ptr> elements;

		blist(std::istream& is, const std::string& filename) {
			read_char_and_check(is, filename, 'l');
			while (is.peek() != 'e') {
				elements.push_back(bncd::read(is, filename));
			}
			read_char_and_check(is, filename, 'e');
		}
		void bprint(std::ostream& os, size_t tabs) const override {
			//skiptabs(os, tabs);
			os << "[";
			newline(os);
			for (auto it = elements.begin(); it != elements.end(); ++it) {
				auto& element = *it;
				skiptabs(os, tabs + 1);
				element->bprint(os, tabs + 1);
			}
			skiptabs(os, tabs);
			os << "]";
			newline(os);
		}
	};
	// dictionary
	struct bdict : public belement {
		std::map<bstring_ptr, belement_ptr> couples;
		std::vector<bstring> order;

		bdict(std::istream& is, const std::string& filename) {
			read_char_and_check(is, filename, 'd');
			while (is.peek() != 'e') {
				auto key = bstring_ptr(static_cast<bstring*>(read(is, filename).release()));
				if (*key == "pieces") {
					auto value = read_pieces(is, filename);
					auto keyv = *key;
					order.push_back(keyv);
					couples.emplace(std::move(key), std::move(value));
				}
				else {
					auto value = read(is, filename);
					auto keyv = *key;
					order.push_back(keyv);
					couples.emplace(std::move(key), std::move(value));
				}
			}
			read_char_and_check(is, filename, 'e');
		}
		void bprint(std::ostream& os, size_t tabs) const override {
			//skiptabs(os, tabs);
			os << "{";
			newline(os);
			for (const auto& key : order) {
				for (const auto& [key_ptr, value_ptr] : couples) {
					if (key == *key_ptr) {
						skiptabs(os, tabs + 1);
						key_ptr->bprint(os);
						os << " => ";
						if (*key_ptr == "pieces") {
							// campo pieces
							value_ptr->print_pieces(os, tabs + 1);
						}
						else {
							value_ptr->bprint(os, tabs + 1);
						}
					}
				}
			}
			/**/
			skiptabs(os, tabs);
			os << "}";
			newline(os);
		}
	};
	bstring_ptr read_pieces(std::istream& is, const std::string& filename) {
		return std::make_unique<bstring>(is, filename, true);
	}
	belement_ptr read(std::istream& is, const std::string& filename) {
		int byte = is.peek();
		if (byte != EOF) {
			switch (byte)
			{
			case 'd':	// dictionary
				return std::make_unique<bdict>(is, filename);
				break;
			case 'l':	// list
				return std::make_unique<blist>(is, filename);
				break;
			case 'i':	// integer
				return std::make_unique<bint>(is, filename);
				break;
			default:	// must be a string
				return std::make_unique<bstring>(is, filename);
				break;
			}
		}
		return nullptr;
	}
}

int main(int argc, char* argv[]) {

	if (argc != 2) { syntax(); }
	std::string filename = argv[1];
	if (!filename.ends_with(".torrent")) { syntax(); }
	std::ifstream is(filename, std::ios::binary);
	check_open(is, filename);

	auto data = bncd::read(is, filename);
	data->bprint(std::cout, 0);

	return EXIT_SUCCESS;
}