#pragma once
#include <charconv>
#include <optional>
#include <string_view>
#include <deque>
#include <string>

template <class T>
inline std::deque<T> splitStr(const std::string_view& delim,const T& src)
{
    std::deque<T> out;
    std::string::size_type start=0;
    while(1)
    {
        auto pz=src.find(delim,start);
        if(pz==std::string::npos)
        {
            out.push_back(src.substr(start));
            break;
        }

        out.push_back(src.substr(start,pz-start));
        start=pz+delim.size();
    }
    return out;
}

inline std::optional<int> to_int(const std::string_view & input)
{
    int out;
    const std::from_chars_result result = std::from_chars(input.data(), input.data() + input.size(), out);
    if(result.ec == std::errc::invalid_argument || result.ec == std::errc::result_out_of_range)
    {
        return std::nullopt;
    }
    return out;
}
