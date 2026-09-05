#pragma once
#include "IDatabase.h"
#include <iostream>
#include <ctime>
#include "DBH.h"

struct CDatabaseMysql: public IDatabase
{

    CDatabaseMysql(const REF_getter<DBH>& dbh, const std::string& _db_name):dbh(dbh),db_name(_db_name)
    {
        dbh->execSimple((QUERY) R"(
                CREATE DATABASE IF NOT EXISTS ?
                CHARACTER SET utf8mb4
                COLLATE utf8mb4_unicode_ci;
          )"<< db_name);
        
        dbh->execSimple((QUERY)R"(
            CREATE TABLE IF NOT EXISTS ?.granule_storage (
                path VARBINARY(32) NOT NULL,
                granule_data BLOB NOT NULL,
                PRIMARY KEY (path)
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
            // auto hash = blake2b_hash(key);
            tr.dbh->execSimple((QUERY) R"(
                REPLACE INTO ?.granule_storage (path, granule_data)
                VALUES (UNHEX('?'), UNHEX('?'));
            )" << db_name << base16::encode(key) << base16::encode(value));
        }
        tr.commit();
        return 0;
    }

    int getGranule(const std::string& k, std::string* v)
    {
        // auto h=blake2b_hash(k);
        auto res=dbh->exec((QUERY)"select granule_data from ?.granule_storage where path = UNHEX('?')"<<db_name<<base16::encode(k));
        if(res->size()!=1)
            return 1;
        if(res->operator[](0).size()!=1)
            return 1;
        *v=res->operator[](0)[0];
        return 0;
    }
    ~CDatabaseMysql()
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