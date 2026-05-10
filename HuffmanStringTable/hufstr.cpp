#include "hufstr.h"
#include <vector>
#include <string>
#include <cstdint>

class hufstr
{
private:
    /* data */
public:
    hufstr()=default;
    ~hufstr()=default;

    std::vector<uint8_t> compress(const std::string& s) const;
    std::string decompress(const std::vector<uint8_t>& v) const;
};