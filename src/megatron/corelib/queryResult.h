#pragma once
#include <map>
#include <vector>
#include <string>
#include "REF.h"

/// DB query result
class QueryResult: public Refcountable
{
public:
    QueryResult():numRows(0), numCols(0) {}
    size_t numRows;
    size_t numCols;
    std::map<int, std::string> fields;
    std::vector< std::vector<std::string> > values;
    size_t size()
    {
        return values.size();
    }
    std::vector<std::string>& operator [](size_t i)
    {
        return values[i];
    };
};
