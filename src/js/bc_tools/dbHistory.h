#pragma once
#include <cstdio>
#include <ctime>
#include <string>
#include "REF.h"
#include "commonError.h"
#include <sys/stat.h>
#include <SQLiteCpp/Database.h>
#define TM_RADIX (3600)

struct DB_history : public Refcountable
{
    std::string base_path;
    DB_history(const std::string &b) : Refcountable("DB_history"), base_path(b)
    {
    }
    void close()
    {
    }

    std::string dbName(int prev)
    {
        struct tm tm;
        time_t t = time(NULL);
        auto tt = t / TM_RADIX;
        tt*=TM_RADIX;
        tt-=TM_RADIX * prev;
        struct tm *ttm = localtime_r(&tt, &tm);
        if (!ttm)
            throw CommonError("if(!ttm)");
        char bn[100];
        snprintf(bn, sizeof(bn), "%s-%04d-%02d-%02d-%02d-%02d", base_path.c_str(), tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,tm.tm_min);

        return bn;
    }
    void write_chunked(const std::string& data)
    {
        auto pn=dbName(0)+".chunked";
        FILE *f=fopen(pn.c_str(),"a");
        fprintf(f,"%ld\n",data.size());
        fwrite(data.data(),data.size(),1,f);
        fclose(f);
    }
    bool writeBlock(const EPOCH_id& epoch, const std::string& prev_root_hash, const std::string& data) {

        write_chunked(data);

        auto pn=base_path+".db";

        SQLite::Database dbs(pn, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

        initDB(dbs);

        SQLite::Statement query(dbs,
                                "INSERT OR REPLACE INTO blocks (prev_root_hash, data, epoch) VALUES (?, ?, ?)"
                               );
        query.bind(1, prev_root_hash.data(),prev_root_hash.size());
        query.bind(2, data.data(),data.size());
        query.bind(3, (int64_t) epoch.container);
        query.exec();
        if(epoch.container>20000)
        {
            dbs.exec("DELETE FROM blocks where epoch<"+ std::to_string(epoch.container)+"-20000");
        }
        return 0;
    }
    bool get(const std::string &prev_root_hash, std::string *v)
    {
        {
            auto pn=base_path+".db";

            try {
                SQLite::Database dbs(pn,  SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);


                initDB(dbs);
                SQLite::Statement query(dbs,
                                        "SELECT data FROM blocks WHERE prev_root_hash = ?"
                                       );
                query.bind(1, prev_root_hash.data(), prev_root_hash.size());

                if (query.executeStep()) {

                    const std::string blob = query.getColumn(0).getString();
                    *v=blob;
                    return 0;
                }
            }
            catch(SQLite::Exception & e)
            {
                logErr2("error open %s",pn);
                return 1;
            }
        }
        return 1;
    }
    bool initDB(SQLite::Database& db) {
        try {
            // Включить WAL для производительности (опционально)
            db.exec("PRAGMA journal_mode=WAL;");
            db.exec("PRAGMA synchronous=NORMAL;");

            db.exec(R"(CREATE TABLE IF NOT EXISTS blocks (
                prev_root_hash BLOB NOT NULL,
                epoch INTEGER NOT NULL,
                data BLOB NOT NULL
            ))");
            db.exec("CREATE INDEX IF NOT EXISTS idx_prev_hash ON blocks(prev_root_hash);");
            db.exec("CREATE INDEX IF NOT EXISTS idx_epoch ON blocks(epoch);");

            return true;
        } catch (...) {
            return false;
        }
    }
};
