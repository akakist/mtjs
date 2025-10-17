#pragma once
#include <set>
#include <map>
#include <time.h>
#include <vector>
#include "msockaddr_in.h"

class _neighbours
{
public:
    std::string dbName;
    std::set<msockaddr_in>sas;
    _neighbours(const std::string &dbName);
    void remove(const msockaddr_in& sa);
    std::vector<msockaddr_in> getAllAndClear();
    void add(const msockaddr_in &sa, int pingTime);
};


