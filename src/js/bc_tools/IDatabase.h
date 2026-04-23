#pragma once
#include <string>
#include "REF.h"
#include "db_to_save.h"
struct IDatabase: public Refcountable
{

    virtual int put_cell(const std::string& k, const std::string& v)=0;
    virtual int get_cell(const std::string& k, std::string* v)=0;
    virtual int write_batch(const _db_to_save &v)=0;

};