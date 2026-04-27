#pragma once
#include "IDatabase.h"
#include <rocksdb/db.h>
#include <iostream>

struct CDatabase: public IDatabase
{
    rocksdb::DB *db;
    int put_cell(const std::string& k, const std::string& v)
    {
        rocksdb::Status s;
        s = db->Put(rocksdb::WriteOptions(), "key", "value");
        if (!s.ok()) std::cerr << "Put failed: " << s.ToString() << "\n";
        return !s.ok();
    }
    int write_batch(const _db_to_save &v)
    {
        rocksdb::WriteBatch batch;
        for(auto& z:v.cells)
        {
            batch.Put(z.first, z.second);
            // logErr2("batch.Put %s size %d",z.first.c_str(), z.second.size());
        }
        rocksdb::Status s=db->Write(rocksdb::WriteOptions(), &batch);
        if (!s.ok())
        {
            throw CommonError("Write failed: %s",s.ToString().c_str());
        }
        logErr2("write batch %d granules",v.cells.size());

        db->Flush(rocksdb::FlushOptions());

        return 0;
    }

    int get_cell(const std::string& k, std::string* v)
    {
        rocksdb::ReadOptions ro;
        ro.verify_checksums = true; // проверять контрольные суммы
        ro.fill_cache = false;      // не засорять блок-кэш при сканах
        auto status=db->Get(ro,k,v);
        if(!status.ok())
        {
            logErr2("get failed %s",k.c_str());
            // throw std::runtime_error("db corrupted");
        }
        return !status.ok();
    }
    CDatabase(const std::string& path)
    {
        db = nullptr;
        rocksdb::Options options;
        options.create_if_missing = true;
        options.atomic_flush = true;            // safer flushes
        options.max_background_jobs = 8;        // tune for your CPU
        options.write_buffer_size = 64 * 1024 * 1024; // 64MB memtable
        options.target_file_size_base = 64 * 1024 * 1024;
        options.level_compaction_dynamic_level_bytes = true;
        options.keep_log_file_num = 3;        // хранить только 3 старых лога
        options.max_log_file_size = 10 * 1024 * 1024; // 10 MB
        rocksdb::Status s = rocksdb::DB::Open(options, path, &db);
        if (!s.ok()) {
            std::cerr << "Open failed: " << s.ToString() << "\n";
            throw CommonError("rocksdb: open filed %s",path.c_str());
        }


    }
    ~CDatabase()
    {
        delete db;
    }
    // int init()
    // {

    //     const std::string path = "/var/lib/mydb";

    //     // Put / Get demo
    //     s = db->Put(rocksdb::WriteOptions(), "key", "value");
    //     if (!s.ok()) std::cerr << "Put failed: " << s.ToString() << "\n";

    //     std::string val;
    //     s = db->Get(rocksdb::ReadOptions(), "key", &val);
    //     if (s.ok()) std::cout << "key=" << val << "\n";
    //     else std::cerr << "Get failed: " << s.ToString() << "\n";

    //     delete db; // Close
    //     return 0;
    // }
};