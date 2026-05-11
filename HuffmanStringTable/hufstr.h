#include <vector>
#include <string>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <unordered_map>

class hufstr
{
public:
	struct elem {
		uint8_t _sym;
		uint8_t _len = 0;
		uint32_t _code = 0;
		elem() {}
		elem(const uint8_t& sym) : _sym(sym) {}
		elem(const uint8_t& sym, const uint8_t& len) : _sym(sym), _len(len) {}
		bool operator<(const elem& rhs) const {
			if (_len < rhs._len)
				return true;
			else if (_len > rhs._len)
				return false;
			else
				return _sym < rhs._sym;
		}
	};
private:
	std::vector<elem> _table;
    std::unordered_map<uint8_t, std::pair<uint8_t, uint32_t>> _c_map;
    std::unordered_map<uint64_t, uint8_t> _d_map;

    bool header(std::ifstream& is) const;
    static uint64_t make_key(uint8_t len, uint64_t code);

public:
    hufstr();
    ~hufstr()=default;

    std::vector<uint8_t> compress(const std::string& s) const;
    std::string decompress(const std::vector<uint8_t>& v) const;
};