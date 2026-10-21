#pragma once
#include "GranuleIO.h"
#include <string>
#include "EventSink.h"
#include "Clock.h"
#include "ContractSourceIO.h"
struct NodeGranuleIO : GranuleIO {
    NodeGranuleIO() : GranuleIO() {}
    bool get(const std::string& key, std::string& out) override {
        (void)key; (void)out;
        // TODO: RocksDB Get
        return false;
    }
    bool exists(const std::string& key) override {
        (void)key;
        // TODO: RocksDB KeyMayExist
        return false;
    }
    bool create(const std::string& key, const std::string& value,
                int64_t create_time, int64_t ttl) override {
        (void)key; (void)value; (void)create_time; (void)ttl;
        // TODO: RocksDB Put (if not exists)
        return false;
    }
    bool set(const std::string& key, const std::string& value) override {
        (void)key; (void)value;
        // TODO: RocksDB Put (if exists)
        return false;
    }
};

struct NodeEventSink : EventSink {
    NodeEventSink() : EventSink() {}
    void emit(const std::string& contract, const std::string& name,
              const std::string& value) override {
        (void)contract; (void)name; (void)value;
        // TODO: блок-очередь событий, flush в БД/лог
    }
};

struct NodeClock : Clock {
    NodeClock() : Clock() {}
    int64_t current_time() override { return 0; }   // TODO: реальное время блока
    int64_t current_block_height() override { return 0; }   // TODO
};

struct NodeContractSourceIO : ContractSourceIO {
    NodeContractSourceIO() : ContractSourceIO() {}
    bool load(const std::string& name, std::string& src) override {
        (void)name; (void)src;
        // TODO: RocksDB / IPFS / HTTP
        return false;
    }
};
