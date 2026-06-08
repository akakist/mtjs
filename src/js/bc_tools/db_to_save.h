#pragma once
#include <map>
#include <string>
// #include "cellable.h"
struct _db_to_save
{
    std::map<std::string, std::string> cells;
    // std::map<void* , std::string> ccells;
    void add(const std::string &dbid, const std::string &body)
    {
        cells[dbid]=body;
    }
    // void add(void* &c, const std::string &body)
    // {
    //     ccells[c]=body;
    // }
    void clear()
    {
        cells.clear();
        // ccells.clear();
    }
};
