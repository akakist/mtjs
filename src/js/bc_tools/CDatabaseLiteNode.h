#pragma once
#include "IDatabase.h"
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/write_batch.h>
#include <rocksdb/table.h>
#include <rocksdb/filter_policy.h>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include "base16.h"
#include "commonError.h"
#include "blake2bHasher.h"

// CDatabaseLiteNode: лайт-нода.
// getGranule: RocksDB (локальный кеш) → HTTP /granule (полная нода).
// write_granules_batch / deleteGranule: только локально (RocksDB).
struct CDatabaseLiteNode : public IDatabase {
    rocksdb::DB* db_ptr = nullptr;
    std::string db_name;
    std::string node_http_addr;

    CDatabaseLiteNode(const std::string& _db_name, const std::string& _addr)
        : db_name(_db_name), node_http_addr(_addr) {
        // RocksDB open (как раньше)
        rocksdb::Options options;
        options.create_if_missing = true;
        rocksdb::Status status = rocksdb::DB::Open(options, db_name, &db_ptr);
        if (!status.ok()) { std::cerr << "CDatabaseLiteNode: " << status.ToString() << std::endl; exit(1); }
    }
    ~CDatabaseLiteNode() { if (db_ptr) { delete db_ptr; db_ptr = nullptr; } }

    std::string getDbName() override { return db_name; }

    int getGranule(const std::string& k, std::string* v) override {
        rocksdb::Status st = db_ptr->Get(rocksdb::ReadOptions(), k, v);
        if (st.ok()) return 0;
        if (!st.IsNotFound()) return 1;
        if (node_http_addr.empty()) return 1;
        httplib::Client cli(node_http_addr);
        cli.set_connection_timeout(2, 0);
        cli.set_read_timeout(5, 0);
        auto res = cli.Get(("/granule?addr=" + base16::encode(k)).c_str());
        if (!res || res->status != 200) return 1;
        try {
            auto j = nlohmann::json::parse(res->body);
            if (!j.contains("body") || !j["body"].is_string()) return 1;
            *v = base16::decode(j["body"].get<std::string>());
            db_ptr->Put(rocksdb::WriteOptions(), k, *v);
            return 0;
        } catch (...) { return 1; }
    }
    int write_granules_batch(const _db_to_save &v) override {
        rocksdb::WriteBatch b;
        for (auto& k : v.deleted) b.Delete(k);
        for (auto& [k, d] : v.cells) b.Put(k, d);
        return db_ptr->Write(rocksdb::WriteOptions(), &b).ok() ? 0 : 1;
    }
    int deleteGranule(const std::string& k) override {
        return db_ptr->Delete(rocksdb::WriteOptions(), k).ok() ? 0 : 1;
    }
};