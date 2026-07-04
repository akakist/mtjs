#pragma once
#include <map>
#include <string>
// #include "cellable.h"
struct _db_to_save
{
    std::map<std::string, std::string> cells;
    void add(const std::string &dbid, const std::string &body)
    {
        cells.insert_or_assign(dbid,body);
    }
    void clear()
    {
        cells.clear();
    }
};
