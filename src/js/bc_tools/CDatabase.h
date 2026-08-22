#pragma once
#include "IDatabase.h"
#include <iostream>
#include <ctime>
#include "DBH.h"

struct CDatabase: public IDatabase
{
    bool getBlock(const BLOCK_id &h,std::string& block)
    {
        auto res=dbh->exec((QUERY)"select block_blob from ?.blocks where prev_state_root_hash = UNHEX('?')"<<db_name<<base16::encode(h.container));
        if(res->size()!=1)
            return 1;
        if(res->operator[](0).size()!=1)
            return 1;
        block=res->operator[](0)[0];
        return 0;
    }
    bool writeBlock(uint64_t epoch, uint64_t block_timestamp,  const std::string& prev_root_hash, const std::string& data)
    {
        /*       CREATE TABLE IF NOT EXISTS ?.blocks (
                height BIGINT UNSIGNED NOT NULL,
                prev_state_root_hash BINARY(32) NOT NULL,
                state_root_hash BINARY(32) NOT NULL,
                block_blob BLOB NOT NULL,
                block_timestamp BIGINT UNSIGNED NOT NULL,
                PRIMARY KEY (height),
                INDEX idx_prev_state_root (prev_state_root_hash),
                INDEX idx_height (height),
                INDEX idx_block_timestamp (block_timestamp)
                
     */
    // logErr2("write block");
        dbh->execSimple((QUERY)R"( REPLACE INTO ?.blocks (height,prev_state_root_hash, block_blob,block_timestamp) VALUES 
            (?,UNHEX('?'),UNHEX('?'),?)
        )"

        <<db_name
        <<epoch
        <<base16::encode(prev_root_hash)
        <<base16::encode(data)
        << block_timestamp
        );
        
        if(epoch>20000)
            dbh->execSimple((QUERY)"delete from ?.blocks where height<?"
            <<db_name
            <<epoch-20000);
        return 0;
    }


    CDatabase(const REF_getter<DBH>& dbh, const std::string& _db_name):dbh(dbh),db_name(_db_name)
    {
        dbh->execSimple((QUERY) R"(
                CREATE DATABASE IF NOT EXISTS ?
                CHARACTER SET utf8mb4
                COLLATE utf8mb4_unicode_ci;
          )"<< db_name);
        
        dbh->execSimple((QUERY)R"(
            CREATE TABLE IF NOT EXISTS ?.granule_storage (
                path_hash BINARY(32) NOT NULL,
                granule_data BLOB NOT NULL,
                PRIMARY KEY (path_hash)
            ) ENGINE=InnoDB
            DEFAULT CHARSET=utf8mb4
            ROW_FORMAT=DYNAMIC;
          )" << db_name);

        dbh->execSimple((QUERY)R"(
            CREATE TABLE IF NOT EXISTS ?.granule_cache (
                granule_hash BINARY(32) NOT NULL,
                granule_data BLOB NOT NULL,
                block_height BIGINT UNSIGNED NOT NULL,
                PRIMARY KEY (granule_hash),
                INDEX idx_block_height (block_height)
            ) ENGINE=InnoDB ROW_FORMAT=DYNAMIC;
           )" << db_name);
        dbh->execSimple((QUERY)R"(
            CREATE TABLE IF NOT EXISTS ?.blocks (
                height BIGINT UNSIGNED NOT NULL,
                prev_state_root_hash BINARY(32) NOT NULL,
                block_blob BLOB NOT NULL,
                block_timestamp BIGINT UNSIGNED NOT NULL,
                PRIMARY KEY (height),
                INDEX idx_prev_state_root (prev_state_root_hash),
                INDEX idx_height (height),
                INDEX idx_block_timestamp (block_timestamp)
                
            ) ENGINE=InnoDB
            DEFAULT CHARSET=utf8mb4
            ROW_FORMAT=DYNAMIC;
           )" << db_name);
    }
    REF_getter<DBH> dbh;
    std::string db_name;

    std::string getDbName()
    {
        return db_name;
    }


    int write_granules_batch(const _db_to_save &v)
    {
        st_TRANSACTION tr(dbh);
        for (const auto& [key, value] : v.cells) {
            auto hash = blake2b_hash(key);
            tr.dbh->execSimple((QUERY) R"(
                REPLACE INTO ?.granule_storage (path_hash, granule_data)
                VALUES (UNHEX('?'), UNHEX('?'));
            )" << db_name << base16::encode(hash.container) << base16::encode(value));
        }
        tr.commit();
        return 0;
    }

    int getGranule(const std::string& k, std::string* v)
    {
        auto h=blake2b_hash(k);
        auto res=dbh->exec((QUERY)"select granule_data from ?.granule_storage where path_hash = UNHEX('?')"<<db_name<<base16::encode(h.container));
        if(res->size()!=1)
            return 1;
        if(res->operator[](0).size()!=1)
            return 1;
        *v=res->operator[](0)[0];
        return 0;
    }
    ~CDatabase()
    {
    }
    std::string timestr()
    {
        struct tm tm;
        time_t t = time(NULL);
        struct tm *ttm = localtime_r(&t, &tm);
        if (!ttm)
            throw CommonError("if(!ttm)");
        char bn[100];
        snprintf(bn, sizeof(bn), "%04d-%02d-%02d-%02d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,tm.tm_min,tm.tm_sec);

        return bn;

    }
};