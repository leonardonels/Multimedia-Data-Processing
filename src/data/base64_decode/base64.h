/**
 * Create files base64.h and base64.cpp that allow to use the following function declaration:
 * 
 * ```cpp
 * std::string base64_decode(const std::string& input);
 * ``` 
 * 
 * The .h file should contain only the non defining declaration of the function, while the definition should be in the .cpp file. Don’t provide a main() function, because one is already defined in the system and will be linked to your code.
 * The function should take a sequence of Base64 encoded bytes as input and provide the decoded sequence as output. The Base64 Content-Transfer-Encoding specification is available in the Support Files section (“Base64 Specification”).
 * If the input is empty, the output will be empty as well. The input sequence will always be valid, so no error checking is required. Proper handling of the final = characters is required.
 */

#include <string>
#include <cstdint>
#include <map>

extern std::map<uint8_t, uint8_t> d_map;
std::string base64_decode(const std::string& input);