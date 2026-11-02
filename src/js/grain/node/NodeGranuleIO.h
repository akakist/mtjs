#pragma once
#include "GranuleIO.h"
#include <string>
#include "jsi/api/EventSink.h"
#include "jsi/api/Clock.h"
#include "jsi/api/ContractSourceIO.h"
#include "IDatabase.h"
#include "REF.h"
#include "contract_constants.h"
#include "cellable.h"

struct NodeGranuleIO : GranuleIO {
    NodeGranuleIO(const REF_getter<IDatabase> &_db) : db(_db) {}
    REF_getter<IDatabase> db;

    bool getGranule(const CONTRACT_id& contract, const CONTRACT_DATA_id& key, std::string& out) override {
        auto d = db->getContractDataNoCreateConst(contract, key);
        if (!d.valid()) return false;
        M_LOCK(d->parent->mx);
        out = d->container;
        return true;
    }

    bool existsGranule(const CONTRACT_id& contract, const CONTRACT_DATA_id& key) override {
        return db->getContractDataNoCreateConst(contract, key).valid();
    }

    std::optional<std::string> createGranule(const CONTRACT_id& contract, const CONTRACT_DATA_id& key,
                                             const std::string& value,
                                             int64_t create_time, int64_t ttl,
                                             Rollback* roll) override {
        auto d = db->getContractDataNoCreateConst(contract, key);
        if (d.valid()) return std::string("create: leaf already exists");
        auto dd = db->getContractDataOrCreate(contract, key, roll, ttl, create_time);
        if (!dd.valid()) return std::string("create failed");
        { M_LOCK(dd->parent->mx); dd->container = value; }
        return std::nullopt;
    }

    std::optional<std::string> setGranule(const CONTRACT_id& contract, const CONTRACT_DATA_id& key,
                                        const std::string& value,
                                        int64_t now, int64_t ttl,
                                        Rollback* roll) override {
        auto d = db->getContractDataNoCreate(contract, key);
        if (!d.valid()) return std::string("granule not exists");
        d->setDirty(roll);
        {
            M_LOCK(d->parent->mx);
            d->container = value;
            d->create_time = now;   // продление: create_time = now
            // d->ttl = ttl;        // ttl не меняется (старый)
        }
        return std::nullopt;
    }
    bool getContractBalance(const CONTRACT_id& c, int64_t& out) override {
        auto b = db->getContractBalanceNoCreateConst(c);
        if (!b.valid()) return false;
        out = b->getAmount();
        return true;
    }
    std::optional<std::string> addContractBalance(const CONTRACT_id& c, int64_t delta, Rollback* roll) override {
        auto b = db->getContractBalanceOrCreate(c, roll);
        if (!b.valid()) return std::string("balance create failed");
        b->addAmount(delta);
        return std::nullopt;
    }

    std::optional<std::string> deleteGranule(const CONTRACT_id& contract, const CONTRACT_DATA_id& key,
                                             Rollback* roll,
                                             std::set<REF_getter<data_base>>* deleted_granules) override {
        auto d = db->getContractDataNoCreate(contract, key);
        if (!d.valid()) return std::string("granule not exists");
        deleted_granules->insert(d.get());
        return std::nullopt;
    }
    int64_t getGranuleTTL(const CONTRACT_id& contract, const CONTRACT_DATA_id& key) override {
        auto d = db->getContractDataNoCreateConst(contract, key);
        if (!d.valid()) return 0;
        M_LOCK(d->parent->mx);
        return d->ttl;
    }
};


struct NodeContractSourceIO : ContractSourceIO {
    REF_getter<IDatabase> db = nullptr;
    NodeContractSourceIO(const REF_getter<IDatabase> &_db) : db(_db) {}

    bool load(const CONTRACT_id& name, std::string& src) override {
        auto cc = db->getContractNoCreateConst(name);
        if (!cc.valid()) {
            logErr2("if(!cc.valid()) %s", name.container.c_str());
            return false;
        }
        src = cc->src;
        return true;
    }
};