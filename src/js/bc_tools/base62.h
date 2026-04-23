#pragma once
#include <string>
namespace base62
{
    std::string encode(const std::string& data);
    std::string decode(const std::string& str);
} // namespace base62