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

struct CNode_prod: public CNode
{
    CNode_prod(){}
    REF_getter<bc_granule> data=nullptr;
    REF_getter<bc_activity> act=nullptr;
    Rollback *roll=nullptr;
    void set(const std::string &s)
    {
        data->container=s;
        data->setDirty(roll);
    }
    void touch(time_t t)
    {
        act->update_time(t,roll);
    }
    std::string get()
    {
        return data->container;
    }
    int ttl()
    {
        return act->ttl;
    }
    void update_time(time_t t, Rollback* r)
    {
        act->update_time(t,r);
    }
    void setDirty(Rollback* r)
    {
        data->setDirty(r);
    }


};
struct NodeGranuleIO : GranuleIO {
    NodeGranuleIO(const REF_getter<IDatabase> &_db) : db(_db) {}
    REF_getter<IDatabase> db;


    bool getGranule(Cellable* root, const CONTRACT_id& contract, const CONTRACT_DATA_id& key, std::string& out) override {
        MUTEX_INSPECTOR;                                            
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
        MUTEX_INSPECTOR;                                            
        {
            auto d = cells::getContractDataNoCreateConst(root,db.get(),contract, key);
            if (d.valid()) return std::string("create: leaf already exists");
        }
        auto dd = cells::getContractDataOrCreate(root,db.get(),contract, key, roll, ttl, create_time);
        if (!dd.valid()) return std::string("create failed");
        {  dd->container = value; }
        {
            auto actd = cells::getActivityOrCreate(root, db.get(), dd->getDbId(),{},ttl,create_time,roll);
            if(!actd.valid())
                throw CommonError("if(!act.valid())");
            actd->update_time(create_time, roll);
            return std::nullopt;
        }
    }

    std::optional<std::string> setGranule(Cellable* root,const CONTRACT_id& contract, const CONTRACT_DATA_id& key,
                                        const std::string& value,
                                        int64_t now, 
                                        Rollback* roll) override {
        MUTEX_INSPECTOR;                                            
        auto d = cells::getContractDataNoCreate(root,db.get(),contract, key);
        if (!d.valid()) return std::string("granule not exists");
        d->setDirty(roll);
        d->container = value;   // <-- ОБНОВИТЬ ЗНАЧЕНИЕ
        auto actd=cells::getActivityNoCreate(root,db.get(),d->getDbId());
        actd->update_time(now,roll);

        return std::nullopt;
    }
    std::optional<std::string> addContractBalance(Cellable* root, const CONTRACT_id& c, int64_t delta, Rollback* roll) override {
        MUTEX_INSPECTOR;                                            
        auto b = cells::getContractBalanceOrCreate(root, db.get(), c, roll);
        if (!b.valid()) return std::string("balance create failed");
        b->addAmount(delta);
        b->setDirty(roll);   // ← ОБЯЗАТЕЛЬНО (diff + calc_tree_hash)
        logErr2("addContractBalance: contract=%s delta=%lld amount=%lld", c.container.c_str(), (long long)delta, (long long)b->getAmount());
        return std::nullopt;
    }
    bool getContractBalance(Cellable* root, const CONTRACT_id& c, int64_t& out) override {
        MUTEX_INSPECTOR;                                            
        auto b = cells::getContractBalanceNoCreateConst(root, db.get(), c);
        if (!b.valid()) { logErr2("getContractBalance: %s NOT FOUND", c.container.c_str()); return false; }
        out = b->getAmount();
        logErr2("getContractBalance: contract=%s amount=%lld", c.container.c_str(), (long long)out);
        return true;
    }
    std::optional<std::string> deleteGranule(Cellable* root, const CONTRACT_id& contract, const CONTRACT_DATA_id& key,
                                             Rollback* roll,
                                             std::set<REF_getter<data_base>>* deleted_granules) override {
        MUTEX_INSPECTOR;                                            
        auto d = cells::getContractDataNoCreate(root,db.get(), contract, key);
        if (!d.valid()) return std::string("granule not exists");
        deleted_granules->insert(d.get());
        return std::nullopt;
    }
    int64_t getGranuleTTL(Cellable* root, const CONTRACT_id& contract, const CONTRACT_DATA_id& key) override {
        MUTEX_INSPECTOR;                                            
        auto d = cells::getContractDataNoCreateConst(root, db.get(), contract, key);
        if (!d.valid()) return 0;
        auto actd=cells::getActivityNoCreate(root,db.get(),d->getDbId());
        return actd->ttl;
    }
    REF_getter<CNode> createCNode(Cellable* root,const CONTRACT_id& c, const CONTRACT_DATA_id& cd,
                                                     int64_t create_time, int64_t ttl,
                                                     Rollback* r)
    {
        auto dbid= blake2b_hash(cells::getContractDataKey(c,cd));
        auto d=cells::getContractDataNoCreate(root,db.get(),c,cd);
        if(d.valid())
            throw CommonError("if(d.valid())#4763@#"); /// error data already exists
        REF_getter<CNode_prod> ret=new CNode_prod;
        ret->data=cells::getContractDataOrCreate(root,db.get(),c,cd,r,ttl,create_time);
        if(!ret->data.valid())
            throw CommonError("if(!ret->data.valid()) @@@#$$");
        ret->data->setDirty(r);
        ret->act=cells::getActivityOrCreate(root,db.get(),dbid.container,{},ttl,create_time,r);
        if(!ret->act.valid())
            throw CommonError("if(!act.valid()) @#886");
        ret->act->setDirty(r);

        return ret.get();
    }
    REF_getter<CNode> getCNode(Cellable* root,const CONTRACT_id& c, const CONTRACT_DATA_id& cd)
    {
        auto dbid= blake2b_hash(cells::getContractDataKey(c,cd));
        REF_getter<CNode_prod> ret=new CNode_prod;
        ret->data=cells::getContractDataNoCreate(root,db.get(),c,cd);
        if(!ret->data.valid())
            return nullptr; /// error not already exists
        ret->act=cells::getActivityNoCreate(root,db.get(),dbid.container);
        if(!ret->act.valid())
            throw CommonError("if(!act.valid()) @#886");

        return ret.get();
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