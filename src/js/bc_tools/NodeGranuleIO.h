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
#include "cells.h"

struct NodeGranuleIO : GranuleIO {
    NodeGranuleIO(const REF_getter<IDatabase> &_db) : db(_db) {}
    REF_getter<IDatabase> db;

    bool getGranule(Cellable* root, const CONTRACT_id& contract, const CONTRACT_DATA_id& key, std::string& out) override {
        auto d = cells::getContractDataNoCreateConst(root, db.get(), contract, key);
        if (!d.valid()) return false;
        
        out = d->container;
        return true;
    }

    bool existsGranule(Cellable* root,const CONTRACT_id& contract, const CONTRACT_DATA_id& key) override {
        return cells::getContractDataNoCreateConst(root,db.get(), contract, key).valid();
    }

    std::optional<std::string> createGranule(Cellable* root,const CONTRACT_id& contract, const CONTRACT_DATA_id& key,
                                             const std::string& value,
                                             int64_t create_time, int64_t ttl,
                                             Rollback* roll) override {
        auto d = cells::getContractDataNoCreateConst(root,db.get(),contract, key);
        if (d.valid()) return std::string("create: leaf already exists");
        auto dd = cells::getContractDataOrCreate(root,db.get(),contract, key, roll, ttl, create_time);
        if (!dd.valid()) return std::string("create failed");
        {  dd->container = value; }
        {
            auto act = cells::getContractActivityNoCreate(root, db.get(), contract);
        if (act.valid()) {
            
            int64_t until = create_time + ttl;
            act->update_time(until);
            act->setDirty(roll);
        }
            return std::nullopt;
        }
    }

    std::optional<std::string> setGranule(Cellable* root,const CONTRACT_id& contract, const CONTRACT_DATA_id& key,
                                        const std::string& value,
                                        int64_t now, 
                                        Rollback* roll) override {
        auto d = cells::getContractDataNoCreate(root,db.get(),contract, key);
        if (!d.valid()) return std::string("granule not exists");
        d->setDirty(roll);
        {
            
            d->container = value;
            d->update_time(now);   // продление: create_time = now
            // d->ttl = ttl;        // ttl не меняется (старый)
        }
        {
            auto act = cells::getContractActivityNoCreate(root, db.get(), contract);
            if (act.valid()) {
                
                // act->update_time = now + ttl;
                int64_t until = now + d->ttl;
                act->update_time(until);
                act->setDirty(roll);
            }
        }
        return std::nullopt;
    }
    // bool getContractBalance(Cellable* root,const CONTRACT_id& c, int64_t& out) override {
    //     auto b = cells::getContractBalanceNoCreateConst(root,db.get(),c);
    //     if (!b.valid()) return false;
    //     out = b->getAmount();
    //     return true;
    // }
    std::optional<std::string> addContractBalance(Cellable* root, const CONTRACT_id& c, int64_t delta, Rollback* roll) override {
        auto b = cells::getContractBalanceOrCreate(root, db.get(), c, roll);
        if (!b.valid()) return std::string("balance create failed");
        b->addAmount(delta);
        b->setDirty(roll);   // ← ОБЯЗАТЕЛЬНО (diff + calc_tree_hash)
        logErr2("addContractBalance: contract=%s delta=%lld amount=%lld", c.container.c_str(), (long long)delta, (long long)b->getAmount());
        return std::nullopt;
    }
    bool getContractBalance(Cellable* root, const CONTRACT_id& c, int64_t& out) override {
        auto b = cells::getContractBalanceNoCreateConst(root, db.get(), c);
        if (!b.valid()) { logErr2("getContractBalance: %s NOT FOUND", c.container.c_str()); return false; }
        out = b->getAmount();
        logErr2("getContractBalance: contract=%s amount=%lld", c.container.c_str(), (long long)out);
        return true;
    }
    std::optional<std::string> deleteGranule(Cellable* root, const CONTRACT_id& contract, const CONTRACT_DATA_id& key,
                                             Rollback* roll,
                                             std::set<REF_getter<data_base>>* deleted_granules) override {
        auto d = cells::getContractDataNoCreate(root,db.get(), contract, key);
        if (!d.valid()) return std::string("granule not exists");
        deleted_granules->insert(d.get());
        return std::nullopt;
    }
    int64_t getGranuleTTL(Cellable* root, const CONTRACT_id& contract, const CONTRACT_DATA_id& key) override {
        auto d = cells::getContractDataNoCreateConst(root, db.get(), contract, key);
        if (!d.valid()) return 0;
        
        return d->ttl;
    }
};


struct NodeContractSourceIO : ContractSourceIO {
    REF_getter<IDatabase> db = nullptr;
    NodeContractSourceIO(const REF_getter<IDatabase> &_db) : db(_db) {}

    bool load(Cellable* root, const CONTRACT_id& name, std::string& src) override {
        auto cc = cells::getContractNoCreateConst(root,db.get(),name);
        if (!cc.valid()) {
            logErr2("if(!cc.valid()) %s", name.container.c_str());
            return false;
        }
        src = cc->src;
        return true;
    }
};