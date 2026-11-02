// gc_snapshot <snapshot_dir> <now_sec> [out.txt]
// iterator (snapshot RocksDB, потоково) → ключи 32 → Cellable unpack → create_time + ttl < now → dbId hex
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include "cellable.h"
#include "inBuffer.h"
#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "usage: gc_snapshot <snapshot_dir> <now_sec> [out.txt]\n";
        return 1;
    }
    std::string snap = argv[1];
    int64_t now = std::atoll(argv[2]);
    std::string out_path = (argc >= 4) ? argv[3] : "expired.txt";

    rocksdb::Options opts;
    rocksdb::DB* db = nullptr;
    rocksdb::Status s = rocksdb::DB::OpenForReadOnly(opts, snap, &db);
    if (!s.ok()) { std::cerr << "open snapshot: " << s.ToString() << "\n"; return 1; }

    std::ofstream out(out_path);
    std::unique_ptr<rocksdb::Iterator> it(db->NewIterator(rocksdb::ReadOptions()));
    size_t scanned = 0, expired = 0;
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        auto key = it->key().ToString();
        if (key.size() != 32) continue;              // только листья
        scanned++;
        auto val = it->value().ToString();
        Cellable c(nullptr, key);
        inBuffer in(val);
        c.unpack_mx(in);                              // children_hashes_mx + data
        if (c.data.valid() && c.data->ttl > 0
            && c.data->create_time + c.data->ttl < now) {
            out << base16::encode(key) << "\n";
            expired++;
        }
        // c — scope (refcount 0 — delete)
    }
    out.close();
    delete db;
    std::cerr << "scanned=" << scanned << " expired=" << expired << " -> " << out_path << "\n";
    return 0;
}
