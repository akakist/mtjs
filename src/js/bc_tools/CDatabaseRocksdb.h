#pragma once
#include "IDatabase.h"
#include <iostream>
#include <ctime>
// #include "DBH.h"
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/write_batch.h>
#include <rocksdb/table.h>
#include <rocksdb/filter_policy.h>
struct CDatabaseRocksdb: public IDatabase
{

    CDatabaseRocksdb(const std::string& _db_name):db_name(_db_name)
    {

        rocksdb::Options options;
        #ifdef KALL
        options.create_if_missing = true;

        // Оптимизации для твоего кейса (мелкие записи, SSD)
        options.write_buffer_size = 64 * 1024 * 1024; // 64 MB memtable
        options.max_write_buffer_number = 3;
        options.target_file_size_base = 64 * 1024 * 1024;
        // options.level_zero_file_num_compaction_trigger = 4;
        options.compression = rocksdb::kLZ4Compression;
        #endif
        options.create_if_missing = true;
        // rocksdb::Options options;

        // 1. Memtable (буфер записи) — минимизируем
        options.write_buffer_size = 16 * 1024 * 1024; // 16 MB (вместо 64 MB)
        options.max_write_buffer_number = 2; // Максимум 2 memtable = 32 MB

        // 2. Block cache (кэш чтения) — основной потребитель
        rocksdb::LRUCacheOptions cache_opts;
        cache_opts.capacity = 256 * 1024 * 1024; // 256 MB (вместо 512 MB)
        cache_opts.num_shard_bits = 4; // Меньше шардов = меньше оверхед
        std::shared_ptr<rocksdb::Cache> block_cache = rocksdb::NewLRUCache(cache_opts);

        rocksdb::BlockBasedTableOptions table_opts;
        table_opts.block_cache = block_cache;
        table_opts.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10, false)); // Bloom filter
        options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_opts));

        // 3. WAL — минимизируем
        options.max_total_wal_size = 32 * 1024 * 1024; // 32 MB (вместо 128 MB)

        // 4. Compaction — агрессивнее сбрасываем на диск
        options.level0_file_num_compaction_trigger = 2; // Начинаем compaction раньше
        options.target_file_size_base = 32 * 1024 * 1024; // 32 MB файлы (вместо 64 MB)

        // 5. Дополнительные оптимизации для экономии памяти
        options.optimize_filters_for_hits = true; // Меньше памяти на фильтры
        options.compaction_pri = rocksdb::kMinOverlappingRatio; // Эффективнее compaction        

        rocksdb::Status status = rocksdb::DB::Open(options, db_name, &db_ptr);

        if (!status.ok()) {
            std::cerr << "Не удалось открыть RocksDB: " << status.ToString() << std::endl;
            exit(1);
        }



    }
    rocksdb::DB* db_ptr = nullptr;

    std::string db_name;

    std::string getDbName()
    {
        return db_name;
    }


    int write_granules_batch(const _db_to_save &v)
    {
        rocksdb::WriteBatch batch;

        for (const auto& [key, data] : v.cells) {
            // rocksdb::Slice key_slice(reinterpret_cast<const char*>(key.data()), key.size());
            // rocksdb::Slice value_slice(reinterpret_cast<const char*>(data.data()), data.size());
            batch.Put(key, data);
        }

        rocksdb::Status status = db_ptr->Write(rocksdb::WriteOptions(), &batch);

        if (!status.ok()) {
            std::cerr << "Ошибка записи батча: " << status.ToString() << std::endl;
            return false;
        }

        return true;
        return 0;
    }
// #include <iostream>
// #include <vector>
// #include <array>
// #include <memory>

// // Размер гранулы: 16 хешей × 32 байта = 512 байт
// constexpr size_t GRANULE_SIZE = 512;

// Ключ в БД — путь в дереве (8 нибблов = 4 байта для 8 уровней)

/// Создание подключения к RocksDB

// /// Чтение одной гранулы по ключу
// bool read_granule(rocksdb::DB* db, const GranuleKey& key, GranuleData& out_data) {
//     std::string value;
//     rocksdb::Slice key_slice(reinterpret_cast<const char*>(key.data()), key.size());
    
//     rocksdb::Status status = db->Get(rocksdb::ReadOptions(), key_slice, &value);

//     if (status.ok()) {
//         if (value.size() == GRANULE_SIZE) {
//             std::memcpy(out_data.data(), value.data(), GRANULE_SIZE);
//             return true;
//         } else {
//             std::cerr << "Неверный размер гранулы: " << value.size() << std::endl;
//             return false;
//         }
//     } else if (status.IsNotFound()) {
//         return false;
//     } else {
//         std::cerr << "Ошибка чтения гранулы: " << status.ToString() << std::endl;
//         return false;
//     }
// }

/// Запись батча гранул (атомарно, после консенсуса)
// bool write_granules_batch(rocksdb::DB* db, const std::vector<std::pair<GranuleKey, GranuleData>>& granules) {
//     rocksdb::WriteBatch batch;

//     for (const auto& [key, data] : granules) {
//         rocksdb::Slice key_slice(reinterpret_cast<const char*>(key.data()), key.size());
//         rocksdb::Slice value_slice(reinterpret_cast<const char*>(data.data()), data.size());
//         batch.Put(key_slice, value_slice);
//     }

//     rocksdb::Status status = db->Write(rocksdb::WriteOptions(), &batch);

//     if (!status.ok()) {
//         std::cerr << "Ошибка записи батча: " << status.ToString() << std::endl;
//         return 1;
//     }

//     return 0;
// }

/// Удаление гранул батчем
    bool delete_granules_batch(rocksdb::DB* db, const std::vector<std::string>& keys) {
        rocksdb::WriteBatch batch;

        for (const auto& key : keys) {
            // rocksdb::Slice key_slice(reinterpret_cast<const char*>(key.data()), key.size());
            batch.Delete(key);
        }

        rocksdb::Status status = db->Write(rocksdb::WriteOptions(), &batch);

        if (!status.ok()) {
            std::cerr << "Ошибка удаления батча: " << status.ToString() << std::endl;
            return 1;
        }

        return 0;
    }

    int getGranule(const std::string& k, std::string* v)
    {
    //   std::string value;
    // rocksdb::Slice key_slice(reinterpret_cast<const char*>(key.data()), key.size());
        
        rocksdb::Status status = db_ptr->Get(rocksdb::ReadOptions(), k, v);

        if (status.ok()) {
            // if (value.size() == GRANULE_SIZE) {
            //     std::memcpy(out_data.data(), value.data(), GRANULE_SIZE);
            //     return true;
            // } else {
            //     std::cerr << "Неверный размер гранулы: " << value.size() << std::endl;
            //     return false;
            // }
            return 0;
        } else if (status.IsNotFound()) {
            return 1;
        } else {
            std::cerr << "Ошибка чтения гранулы: " << status.ToString() << std::endl;
            return false;
        }
        return 0;
    }
    ~CDatabaseRocksdb()
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