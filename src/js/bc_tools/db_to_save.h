#pragma once
#include <map>
#include <string>
// #include "cellable.h"
struct _db_to_save
{
    std::map<std::string, std::string> cells;
    void add(const std::string &dbid, const std::string &body) {
        cells[dbid]=body;
    }
    void add(_db_to_save& d) {
        for(auto& z:d.cells) cells[z.first]=z.second;
    }
    size_t size() {
        size_t sz=0;
        for(auto& z: cells) { sz += z.first.size(); sz += z.second.size(); }
        return sz;
    }
    // листья: ключ = 32 байта (1+1+1+1+28); промежуточные 1-4 — не считаем
    size_t size_leaves() const {
        size_t sz=0;
        for(auto& z: cells) {
            if (z.first.size() == 32) {
                sz += z.first.size();
                sz += z.second.size();
            }
        }
        return sz;
    }
    void clear() { cells.clear(); }
};

