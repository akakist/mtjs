#pragma once
#include <string>
#include <stdlib.h>
#include <stdexcept>

inline std::string getenv2(const char* name)
{
    auto *p=getenv(name);
    if(!p)
        throw std::runtime_error((std::string)"genenv failed "+name);
    return p;
}
inline std::string getenv2(const std::string& name)
{
    auto *p=getenv(name.c_str());
    if(!p)
        throw std::runtime_error((std::string)"genenv failed "+name);
    return p;
}
