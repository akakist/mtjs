#pragma once
#include <string>
namespace base62
{
    // std::string encode(const std::string& data);
    // std::string decode(const std::string& str);

std::string encode(const std::string &input) ;
std::string encode(const std::vector<uint8_t> &input) ;
std::string encode(const uint8_t* data, size_t length) ;
std::string decode(const std::string& data);
std::string decode(const std::vector<uint8_t>& data);
std::string decode(const uint8_t* data, size_t length) ;
   static uint8_t hex_char_to_value(uint8_t c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        throw std::invalid_argument("Invalid hex character");
    }
    
    static bool is_hex_char(char c) {
        return (c >= '0' && c <= '9') ||
               (c >= 'a' && c <= 'f') ||
               (c >= 'A' && c <= 'F');
    }
} // namespace base62