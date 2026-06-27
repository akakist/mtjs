#pragma once
#include <string>
#include "REF.h"
#include "db_to_save.h"
#include "EPOCH_id.h"
struct IDatabase: public Refcountable
{
    virtual void close() =  0;

    virtual bool compactRange() = 0;

    virtual int put_cell(const std::string& k, const std::string& v)=0;
    virtual int get_cell(const std::string& k, std::string* v)=0;
    virtual int write_batch(const _db_to_save &v)=0;
    virtual bool create_snapshot(const EPOCH_id&) = 0;

    IDatabase():Refcountable("Idatabse") {}
};