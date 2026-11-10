#pragma once
#include "GranuleIO.h"
#include <map>
#include <optional>
#include "cellable.h"
struct TestGranuleIO : GranuleIO {
    std::map<std::string, Granule> data;
    TestGranuleIO() : GranuleIO() {}

    std::map<std::string, int64_t> balances;   // contract -> amount

    bool getContractBalance(Cellable*, const CONTRACT_id& c, int64_t& out) override {
        auto it = balances.find(c.container);
        if (it == balances.end()) return false;
        out = it->second;
        return true;
    }
    std::optional<std::string> addContractBalance(Cellable*,const CONTRACT_id& c, int64_t delta, Rollback* roll) override {
        balances[c.container] += delta;
        return std::nullopt;
    }

    bool getGranule(Cellable*,const CONTRACT_id& c, const CONTRACT_DATA_id& d, std::string& out) override {
        printf("get: %s %s\n",c.container.c_str(),d.container.c_str());
        auto it = data.find(c.container+d.container);
        if (it == data.end())
        {
            printf("not found\n");
             return false;
        }
        printf("out: %s\n",it->second.value.c_str());
        out = it->second.value;
        return true;
    }
    std::optional<std::string> deleteGranule(Cellable*,const CONTRACT_id& c, const CONTRACT_DATA_id& d,
                                            Rollback* roll, std::set<REF_getter<data_base>> * deleted_granules)  override {
        auto it = data.find(c.container + d.container);
        if (it == data.end()) return std::string("not found");
        data.erase(it);   // сразу (rollback — saved/restore)
        return std::nullopt;
    }    
    bool existsGranule(Cellable*,const CONTRACT_id& c, const CONTRACT_DATA_id& d) override {
        return data.find(c.container+d.container) != data.end();
    }
    std::optional<std::string> createGranule(Cellable*,const CONTRACT_id& c, const CONTRACT_DATA_id& d, const std::string& value,
                int64_t create_time, int64_t ttl, Rollback* roll) override {
        if (data.find(c.container+d.container) != data.end())
        {
            printf("create: already exists");
            return std::string("already exists");

        }
        data[c.container+d.container] = Granule{ value, create_time, ttl };
        return std::nullopt;
    }
    std::optional<std::string> setGranule(Cellable*,const CONTRACT_id& c, const CONTRACT_DATA_id& d,
                                        const std::string& value,
                                        int64_t now, 
                                        Rollback* roll) override {
        auto it = data.find(c.container + d.container);
        if (it == data.end()) return std::string("not found");
        it->second.value = value;
        it->second.create_time = now;
        return std::nullopt;
    }
    int64_t getGranuleTTL(Cellable*,const CONTRACT_id& c, const CONTRACT_DATA_id& d) override {
        auto it = data.find(c.container + d.container);
        if (it == data.end()) return 0;
        return it->second.ttl;
    }
    REF_getter<CNode> createCNode(Cellable*,const CONTRACT_id& c, const CONTRACT_DATA_id& cd,
                                                     int64_t create_time, int64_t ttl,
                                                     Rollback* r)
    {
        return nullptr;
    }
    REF_getter<CNode> getCNode(Cellable*,const CONTRACT_id& c, const CONTRACT_DATA_id& cd)
    {
        return nullptr;
    }
    void update_contract_time(Cellable*,const CONTRACT_id& c, time_t t, Rollback* roll)
    {
        
    }

};



struct TestContractSourceIO : ContractSourceIO {
    TestContractSourceIO() : ContractSourceIO() {}

    bool load(Cellable*, const CONTRACT_id& name, std::string& src) override {
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


struct TestMsgSource : IMsgSource {
    jsiMsg m;
    TestMsgSource(const jsiMsg& _m) : m(_m) {}
    std::string get_sender() const override { return m.sender; }
    int64_t get_value() const override { return m.value; }
    int64_t get_gasLimit() const override { return m.gasLimit; }
    int64_t get_gasUsed() const override { return m.gasUsed; }
    int64_t get_blockTimestamp() const override { return m.blockTimestamp; }
    int64_t get_blockHeight() const override { return m.blockHeight; }
    int64_t get_nonce() const override { return m.nonce; }
    int64_t get_chainId() const override { return m.chainId; }
    std::string get_txHash() const override { return m.txHash; }
    int64_t get_txIndex() const override { return m.txIndex; }

    void consume_value(int64_t amount) override { m.value -= amount; }
    void refund_value(int64_t amount) override { m.value += amount; }
    void consume_gas(int64_t amount) override { m.gasUsed += amount; }
};

