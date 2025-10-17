#pragma once
#include <regex>
#include <set>
#include <vector>
#include <deque>

inline std::vector<std::string> resplit(const std::string &s, const std::regex &sep_regex)
{
    std::sregex_token_iterator iter(s.begin(), s.end(), sep_regex, -1);
    std::sregex_token_iterator end;
    return {iter, end};
}

inline std::deque<std::string> resplitDQ(const std::string &s, const std::regex &sep_regex)
{
    std::sregex_token_iterator iter(s.begin(), s.end(), sep_regex, -1);
    std::sregex_token_iterator end;
    return {iter, end};
}

inline std::set<std::string> resplitSET(const std::string &s, const std::regex &sep_regex)
{
    std::sregex_token_iterator iter(s.begin(), s.end(), sep_regex, -1);
    std::sregex_token_iterator end;
    return {iter, end};
}
