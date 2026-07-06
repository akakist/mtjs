#pragma once
#include <string>
#include <unordered_map>
#include <algorithm>

inline std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t");
    size_t end   = s.find_last_not_of(" \t");
    if (start == std::string::npos)
        return "";
    return s.substr(start, end - start + 1);
}

inline std::unordered_map<std::string, std::string> parse_cookie(const std::string& cookie) {
    std::unordered_map<std::string, std::string> result;

    size_t start = 0;
    while (start < cookie.size()) {
        size_t end = cookie.find(';', start);
        if (end == std::string::npos)
            end = cookie.size();

        std::string pair = trim(cookie.substr(start, end - start));

        size_t eq = pair.find('=');
        if (eq != std::string::npos) {
            std::string key = trim(pair.substr(0, eq));
            std::string value = trim(pair.substr(eq + 1));
            result[key] = value;
        }

        start = end + 1;
    }

    return result;
}
