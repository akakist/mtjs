#pragma once
#include <string>
#include <map>
struct Cellable;

struct Rollback
{
    std::map<Cellable*, std::string> data;
    size_t size()
    {
        size_t ret;
        for(auto &z: data)
            ret+=z.second.size();
        return ret;
    }
};
