#pragma once
#include <map>
#include <string>
#include <set>
// #include "cellable.h"
struct _db_to_save
{
    std::map<std::string, std::string> cells;
    std::set<std::string> deleted; 
    size_t size() {
        size_t sz = 0;
        for (auto& z : cells)   { sz += z.first.size(); sz += z.second.size(); }
        for (auto& z : deleted) { sz += z.size(); }
        return sz;
    }
    size_t size_leaves() const {
        size_t sz = 0;
        for (auto& z : cells)   { if (z.first.size() == 32) { sz += z.first.size(); sz += z.second.size(); } }
        for (auto& z : deleted) { if (z.size() == 32) sz += z.size(); }
        return sz;
    }
    void clear() { cells.clear(); deleted.clear(); }

    void add(const std::string &dbid, const std::string &body) {
        cells[dbid]=body;
    }
    void del(const std::string &dbid) {
        deleted.insert(dbid);
    }
    void add(_db_to_save& d) {
        for (auto& z : d.cells)   cells[z.first] = z.second;
        for (auto& z : d.deleted) deleted.insert(z);
    }
};

