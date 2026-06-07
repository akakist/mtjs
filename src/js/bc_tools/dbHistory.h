#pragma once
#include <cstdio>
#include <ctime>
#include <map>
#include <string>
#include "IDatabase.h"
#include "REF.h"
#include "commonError.h"
#include "CDatabase.h"
#include <sys/stat.h>
#include <SQLiteCpp/Database.h>
#define TM_RADIX (3600)
/*void Node::Service::initDB()
{
    SQLite::Database dbs(sqlite_pn, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

    dbs.exec(R"( CREATE TABLE IF NOT EXISTS  blocks (
    epoch INTEGER UNIQUE,
    prev_root_hash BLOB NOT NULL,
    data BLOB NOT NULL,
    date INTEGER NOT NULL
        )
        )");
    dbs.exec("CREATE INDEX IF NOT EXISTS   idx_prev_hash ON blocks(prev_root_hash);");
    dbs.exec("CREATE INDEX IF NOT EXISTS idx_epoch ON blocks(epoch)");
}*/

struct DB_history : public Refcountable
{
    std::string base_path;
    // std::map<std::string, std::unique_ptr<SQLite::Database>> container;
    DB_history(const std::string &b) : Refcountable("DB_history"), base_path(b)
    {
    }
    void close()
    {
        // container.clear();
        // for(auto& z: container)
        // {
        //     z.second->close();
        // }
    }
    // bool isDBOpen(const std::unique_ptr<SQLite::Database>& db_) const {
    //     if (!db_) return false;
        
    //     // Проверяем, что sqlite3* не nullptr
    //     sqlite3* handle = db_->getHandle();
    //     if (!handle) return false;
        
    //     // Дополнительно проверяем, что соединение живо
    //     return (sqlite3_db_status(handle, SQLITE_DBSTATUS_SCHEMA_VERSION, 
    //                                nullptr, nullptr, 0) == SQLITE_OK);
    // }
    // std::unique_ptr<SQLite::Database> getDb(const std::string &pn, int mode)
    // {

    //     if(mode==O_RDONLY)
    //     {
    //         auto db_ = std::make_unique<SQLite::Database>(
    //                 pn, 
    //                 SQLite::OPEN_READONLY 
    //             );
    //         if(!isDBOpen(db_))
    //         {
    //             return NULL;
    //         }
    //         return db_;

    //     }
    //     else
    //     {
    //         auto db_ = std::make_unique<SQLite::Database>(
    //                 pn, 
    //                 SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE
    //             );
    //         if(!isDBOpen(db_))
    //         {
    //             return NULL;
    //         }
    //         return db_;

    //     }
    //     // auto it = container.find(pn);
    //     // if (it != container.end())
    //     //     return it->second;
    //     //     try{
    //     //         if(mode==O_RDONLY)
    //     //         {
    //     //             struct stat st;
    //     //             if(stat(pn.c_str(),&st))
    //     //             {
    //     //                 return NULL;
    //     //             }

    //     //         }
    //     //         REF_getter<IDatabase> d = new CDatabase(pn);
    //     //         container.insert_or_assign(pn, d);
    //     //         return d;
    //     //     }
    //     //     catch(...)
    //     //     {
    //     //         return NULL;
    //     //     }
    //     //     return NULL;
    // }
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
    bool writeBlock(const std::string& prev_root_hash, const std::string& data) {
        
        auto pn=dbName(0);

        SQLite::Database dbs(pn, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

        initDB(dbs);

        SQLite::Statement query(dbs, 
            "INSERT OR REPLACE INTO blocks (prev_root_hash, data) VALUES (?, ?)"
        );
        query.bind(1, prev_root_hash.data(),prev_root_hash.size());
        query.bind(2, data.data(),data.size());  
        query.exec();
        return 0;
    }
    bool get(const std::string &prev_root_hash, std::string *v)
    {
        int n = 0;
        while (true)
        {
            auto pn = dbName(n);

            try{
                SQLite::Database dbs(pn, SQLite::OPEN_READONLY );
                

                initDB(dbs);
                SQLite::Statement query(dbs, 
                "SELECT data FROM blocks WHERE prev_root_hash = ?"
                );
                query.bind(1, prev_root_hash.data(), prev_root_hash.size());
        
                if (query.executeStep()) {
                    // getBlob() возвращает const void*

                    const std::string blob = query.getColumn(0).getString();
                    *v=blob;
                    return 0;
                    // size_t size = query.getColumn(0).getBytes();
                    // const char** bytes = static_cast<const char**>(blob);
                }
            }
            catch(SQLite::Exception & e)
            {
                logErr2("error open %s",pn);
                return 1;
            }
            // auto db = getDb(nm, O_RDONLY);
            n++;
            // if(!db.valid())
            //     return 1;
            // if (db->get_cell(k, v))
            // {
            //     n++;
            // }
            // else
            // {
            //     return 0;
            // }
        }
    }
    bool initDB(SQLite::Database& db) {
        try {
            // Включить WAL для производительности (опционально)
            db.exec("PRAGMA journal_mode=WAL;");
            db.exec("PRAGMA synchronous=NORMAL;");
            
            db.exec(R"(CREATE TABLE IF NOT EXISTS blocks (
                prev_root_hash BLOB NOT NULL,
                data BLOB NOT NULL
            ))");
            db.exec("CREATE INDEX IF NOT EXISTS idx_prev_hash ON blocks(prev_root_hash);");
            
            return true;
        } catch (...) {
            return false;
        }
    }
};