#pragma once
#include "GranuleIO.h"
#include <map>
struct TestGranuleIO : GranuleIO {
    std::map<std::string, Granule> data;
    TestGranuleIO() : GranuleIO() {}

    bool get(const std::string& key, std::string& out) override {
        auto it = data.find(key);
        if (it == data.end()) return false;
        out = it->second.value;
        return true;
    }
    bool exists(const std::string& key) override {
        return data.find(key) != data.end();
    }
    bool create(const std::string& key, const std::string& value,
                int64_t create_time, int64_t ttl) override {
        if (data.find(key) != data.end()) return false;
        data[key] = Granule{ value, create_time, ttl };
        return true;
    }
    bool set(const std::string& key, const std::string& value) override {
        auto it = data.find(key);
        if (it == data.end()) return false;
        it->second.value = value;
        return true;
    }
};


struct TestEventSink : EventSink {
    std::vector<jsiEvent> current_events;
    std::vector<jsiEvent> all_events;

    TestEventSink() : EventSink() {}

    void emit(const std::string& contract, const std::string& name,
              const std::string& value) override {
        current_events.push_back(jsiEvent{ contract, name, value });
    }
};

struct TestClock : Clock {
    int64_t time_ = 0;
    int64_t height_ = 0;
    TestClock() : Clock() {}
    int64_t current_time() override { return time_; }
    int64_t current_block_height() override { return height_; }
    void set(int64_t t, int64_t h = 0) { time_ = t; height_ = h; }
};

struct TestContractSourceIO : ContractSourceIO {
    TestContractSourceIO() : ContractSourceIO() {}

    bool load(const std::string& name, std::string& src) override {
        std::string path = "contracts/" + name + ".js";
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) return false;
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        src.resize(size);
        auto sz = fread(&src[0], 1, size, f);
        fclose(f);
        return sz == (size_t)size;
    }
};

