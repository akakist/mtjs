#pragma once
#include "IDatabase.h"
#include <rocksdb/db.h>
#include <iostream>

struct CDatabase: public IDatabase
{

    rocksdb::DB *db;
    void close()
    {
        db->Close();
    }
    bool compactRange()
    {
        rocksdb::CompactRangeOptions compact_opts;
        // 1. Задаем поведение на нижнем (bottommost) уровне. Чтобы наверняка утрамбовать все данные и удалить старые версии, используем `kForce` [citation:9].
        compact_opts.bottommost_level_compaction = rocksdb::BottommostLevelCompaction::kForce;

// 2. Разрешаем временно увеличить нагрузку на диск, чтобы ускорить процесс.
        compact_opts.allow_write_stall = true;

// 3. Увеличиваем количество подкомпактов для параллельной работы (опционально, если есть свободные ядра).
        compact_opts.max_subcompactions = 4;

        rocksdb::Status s = db->CompactRange(compact_opts, nullptr, nullptr);
        return !s.ok();
    }
    int put_cell(const std::string& k, const std::string& v)
    {
        rocksdb::Status s;
        s = db->Put(rocksdb::WriteOptions(), k, v);
        if (!s.ok()) std::cerr << "Put failed: " << s.ToString() << "\n";
        // db->Flush(rocksdb::FlushOptions());
        return !s.ok();
    }
    int write_batch(const _db_to_save &v)
    {
        rocksdb::WriteBatch batch;
        for(auto& z:v.cells)
        {
            batch.Put(z.first, z.second);
        }
        rocksdb::Status s=db->Write(rocksdb::WriteOptions(), &batch);
        if (!s.ok())
        {
            throw CommonError("Write failed: %s",s.ToString().c_str());
        }

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
            logErr2("get failed %s",base16::encode(k).c_str());
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
//        options.max_open_files=800;
        // options.compaction_on_commit = true;
//        options.level0_file_num_compaction_trigger = 2;
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
};