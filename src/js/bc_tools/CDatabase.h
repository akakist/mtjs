#pragma once
#include "IDatabase.h"
#include <rocksdb/db.h>
#include <rocksdb/utilities/checkpoint.h>
#include <iostream>
#include <ctime>


struct CDatabase: public IDatabase
{

    rocksdb::DB *db;
    std::string path_;
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
        rocksdb::WriteOptions w;
        // w.sync=true;
        s = db->Put(w, k, v);
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
    CDatabase(const std::string& path):path_(path)
    {
        db = nullptr;
        rocksdb::Options options;
        options.create_if_missing = true;
        options.atomic_flush = true;

        // ========== Фоновые потоки ==========
        options.max_background_jobs = 8;        // хорошо
        options.max_background_compactions = 6; // явно выделяем под компакцию

        // ========== Memtable ==========
        options.write_buffer_size = 128 * 1024 * 1024;  // 128MB (было 64)
        options.max_write_buffer_number = 4;              // максимум 4 мемтаблицы
        options.min_write_buffer_number_to_merge = 2;

        // ========== L0 компакция (критично!) ==========
        options.level0_file_num_compaction_trigger = 2;   // было 4 → старт при 2 файлах
        options.level0_slowdown_writes_trigger = 8;       // замедление при 8
        options.level0_stop_writes_trigger = 12;          // останов при 12 (не должно достигаться)

        // ========== Размеры файлов и уровней ==========
        options.target_file_size_base = 64 * 1024 * 1024; // 64MB (ок)
        options.target_file_size_multiplier = 1;
        options.max_bytes_for_level_base = 512 * 1024 * 1024; // 512MB для L1
        options.level_compaction_dynamic_level_bytes = true; // хорошо

        // ========== Логи ==========
        options.keep_log_file_num = 3;
        options.max_log_file_size = 10 * 1024 * 1024;

// ========== Блочный кэш (важно для чтения) ==========
// rocksdb::BlockBasedTableOptions table_options;
// table_options.block_cache = rocksdb::NewLRUCache(512 * 1024 * 1024); // 512 MB cache
// options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));


// std::shared_ptr<rocksdb::Cache> cache = rocksdb::NewLRUCache(512 * 1024 * 1024); // 512MB
// options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(
//     rocksdb::BlockBasedTableOptions{cache}
// ));        
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
//     #include <memory>
// #include <rocksdb/db.h>
// #include <rocksdb/utilities/checkpoint.h>

// rocksdb::DB* db; // уже открытая БД

// Создание снапшота
    bool create_snapshot(const EPOCH_id &epoch) 
    {
        // return true;
    // Создаем умный указатель для Checkpoint. Он сам вызовет delete.
    rocksdb::Checkpoint* checkpoint=NULL;
    
    // Создаем объект Checkpoint
    rocksdb::Status s = rocksdb::Checkpoint::Create(db, &checkpoint);
    if (!s.ok()) {
        std::cerr << "Checkpoint creation failed: " << s.ToString() << std::endl;
        delete checkpoint;
        return false;
    }
    char pn[100];
    snprintf(pn, sizeof(pn),"%s-snapshot-%09lx",path_.c_str(), epoch.container/SNAPSHOT_BLOCKS);
    // std::string snapshot_path=path_+"-snapshot-"+timestr();
    // Создаем снапшот
    // 0 означает, что RocksDB сама решит, делать ли flush
    s = checkpoint->CreateCheckpoint(pn, 0);
    
    if (!s.ok()) {
        std::cerr << "Checkpoint creation failed: " << s.ToString() << std::endl;
        delete checkpoint;
        return false;
    }
    
    delete checkpoint;
    return true;
}
};