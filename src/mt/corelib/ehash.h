#include  <string>
constexpr int ehash(const std::string &s)
{
    return std::hash<std::string> {}(s);
}
