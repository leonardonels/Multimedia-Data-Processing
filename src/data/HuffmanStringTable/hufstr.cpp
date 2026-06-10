#include "hufstr.h"

const std::string MAGICNUMBER = "HUFFMANSTRINGTABLE";

hufstr::hufstr()
{
    std::ifstream is("table.bin", std::ios::binary);
    if(!is)
        exit(1);

    if(!header(is))
        exit(2);

    for(size_t i = 0; i < 256; i++){
        uint8_t sym; uint8_t len;
        is.get(reinterpret_cast<char&>(sym)); is.get(reinterpret_cast<char&>(len));
        _table.emplace_back(sym, len);
    }
    //_table.sort();    // The table should already be sorted

    _c_map[_table[0]._sym] = {_table[0]._len, 0};
    _d_map[make_key(_table[0]._len, 0)] = _table[0]._sym;
    uint8_t prev_len = _table[0]._len; uint32_t prev_code = 0;
    for(size_t i = 1; i < _table.size(); i++){
        uint32_t code = (prev_code + 1) << (_table[i]._len - prev_len);
        _c_map[_table[i]._sym] = {_table[i]._len, code};
        _d_map[make_key(_table[i]._len, code)] = _table[i]._sym;
        prev_len = _table[i]._len;
        prev_code = code;
    }
}

bool hufstr::header(std::ifstream& is) const
{
    char c;
    for(size_t i = 0; i < 18; i++){
        is.get(c);
        if(c!=MAGICNUMBER[i])
            return false;
    }
    return true;
}

uint64_t hufstr::make_key(uint8_t len, uint64_t code) {
    return ((uint64_t)len << 32) | code;
}

std::vector<uint8_t> hufstr::compress(const std::string& s) const
{
    uint64_t buffer = 0;
    size_t bits = 0;
    std::vector<uint8_t> v;

    for(char c : s){
        uint8_t size = _c_map.at(c).first;
        uint32_t val = _c_map.at(c).second;
        buffer = (buffer << size) | (uint64_t)(val & ((1ull << size) - 1));
        bits += size;

        while(bits >= 8){
            bits -= 8;
            v.push_back((uint8_t)((buffer >> (bits)) & 0xFF));
        }
    }
    if (bits > 0) {
        int pad = 8 - bits;
        buffer = (buffer << pad) | ((1ull << pad) - 1);
        v.push_back(static_cast<uint8_t>(buffer & 0xFF));    
    }

    return v;
}

std::string hufstr::decompress(const std::vector<uint8_t>& v) const
{
    uint64_t buffer = 0;
    size_t bits = 0;
    size_t v_pointer = 0;

    std::string out = "";

    uint64_t x = 0;
    uint8_t len = 0;
    while(v_pointer < v.size() || bits > 0){
        if(bits == 0){
            buffer = (buffer << 8) | (uint64_t)(v[v_pointer++]);
            bits += 8;
        }
        bits--;
        uint8_t val = (buffer>>bits) & 1u;

        x = (x << 1) | val;
        len++;

        auto it = _d_map.find(hufstr::make_key(len,x));
        if (it != _d_map.end()){
            out += (char)(it->second);
            x = 0;
            len = 0;
        }
    }

    return out;
}