#pragma once
#include "GranuleIO.h"
#include <map>
#include <optional>
struct TestGranuleIO : GranuleIO {
    std::map<std::string, Granule> data;
    TestGranuleIO() : GranuleIO() {}

    bool get(const CONTRACT_id& c, const CONTRACT_DATA_id& d, std::string& out) override {
        auto it = data.find(c.container+d.container);
        if (it == data.end()) return false;
        out = it->second.value;
        return true;
    }
    bool exists(const CONTRACT_id& c, const CONTRACT_DATA_id& d) override {
        return data.find(c.container+d.container) != data.end();
    }
    std::optional<std::string> create(const CONTRACT_id& c, const CONTRACT_DATA_id& d, const std::string& value,
                int64_t create_time, int64_t ttl, Rollback* roll) override {
        if (data.find(c.container+d.container) != data.end())
            return std::string("already exists");
        data[c.container+d.container] = Granule{ value, create_time, ttl };
        return std::nullopt;
    }
    std::optional<std::string> set(const CONTRACT_id& c, const CONTRACT_DATA_id& d, const std::string& value, Rollback* roll) override {
        auto it = data.find(c.container+d.container);
        if (it == data.end()) return std::string("not found");
        it->second.value = value;
        return std::nullopt;
    }
};


struct TestEventSink : EventSink {
    std::vector<jsiEvent> current_events;
    std::vector<jsiEvent> all_events;

    TestEventSink() : EventSink() {}

    void emit(const CONTRACT_id& contract, const std::string& name,
              const std::string& value) override {
        current_events.push_back(jsiEvent{ contract.container, name, value });
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

    bool load(const CONTRACT_id& name, std::string& src) override {
        std::string path = "contracts/" + name.container + ".js";
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
