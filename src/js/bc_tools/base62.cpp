#include <string>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <stdexcept>
#include <string.h>
#include "base62.h"

std::string base62::encode(const std::string &input) 
{
    return encode((const uint8_t*)input.data(), input.size());
}
std::string base62::encode(const std::vector<uint8_t> &input) 
{
    return encode(input.data(), input.size());
}
std::string base62::encode(const uint8_t* data, size_t length) 
{
 static const char hex_chars[] = "0123456789abcdef";
        std::string result;
        result.reserve(length * 2);
        
        for (size_t i = 0; i < length; i++) {
            result.push_back(hex_chars[data[i] >> 4]);
            result.push_back(hex_chars[data[i] & 0x0F]);
        }
        return result;

}
std::string base62::decode(const std::string& data) 
{
    return decode((const uint8_t*)data.data(), data.size());
}
std::string base62::decode(const std::vector<uint8_t>& data) 
{
    return decode((const uint8_t*)data.data(), data.size());
}

std::string base62::decode(const uint8_t* data, size_t length) 
{

          if (length % 2 != 0) {
            throw std::invalid_argument("Hex string length must be even");
        }
        
        std::string result;
        result.reserve(length / 2);
        
        for (size_t i = 0; i < length; i += 2) {
            uint8_t high = hex_char_to_value(data[i]);
            uint8_t low = hex_char_to_value(data[i + 1]);
            result.push_back((high << 4) | low);
        }
        return result;
}


#ifdef ___1

#include <iostream>

int main() {

    auto enc=base62::encode((const uint8_t*)"Sergey Belyalov", 15);
    std::cout<<"Base62: "<<enc<<std::endl;
    auto dec=base62::decode((const uint8_t*)enc.data(), enc.size());
    std::cout<<"Decoded: "<<std::string((char*)dec.data(),dec.size())<<std::endl;

}
#endif