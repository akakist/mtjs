#pragma once
#include <map>
#include <string>
struct _db_to_save
{
    std::map<std::string, std::string> cells;
    void add(const std::string &dbid, const std::string &body)
    {
        cells[dbid]=body;
    }
    void clear()
    {
        cells.clear();
    }
};
