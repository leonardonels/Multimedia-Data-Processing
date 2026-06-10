#include "hufstr.cpp"
#include <iostream>

void test(hufstr& h, std::string s)
{
    std::cout << "input: " << s << std::endl;

    std::vector<uint8_t> v = h.compress(s);
    std::cout << "compressed: ";
    for(auto e : v){
        for(size_t i=8; i>0; i--){
            std::cout << ((e>>i)&1) << " ";
        }
    }
    std::cout << std::endl;
    std::string o = h.decompress(v);
    std::cout << "decompressed: " << o << std::endl << std::endl;
}

int main()
{
    hufstr h;

    test(h, "aeio");
    test(h, "aeioaeioaeioaeioaeioaeioaeioaeioaeioaeioaeioaeioaeio");

    test(h, "Hello, World!");
    test(h, "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");

    return 0;
}