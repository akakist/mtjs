#pragma once
#include <string>
#include "REF.h"
#include "db_to_save.h"
#include "THASH_id.h"
#include "BLOCK_id.h"
struct IDatabase: public Refcountable
{

    virtual std::string getDbName()=0;

    virtual int getGranule(const std::string& k, std::string* v)=0;
    virtual int write_granules_batch(const _db_to_save &v)=0;
    virtual bool getBlock(const BLOCK_id &h, std::string& block) = 0;
    virtual bool writeBlock(uint64_t epoch, uint64_t block_timestamp,  const std::string& prev_root_hash, const std::string& data)=0;


    IDatabase():Refcountable("Idatabse") {}
};