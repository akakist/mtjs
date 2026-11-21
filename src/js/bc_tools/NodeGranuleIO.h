#pragma once
#include "GranuleIO.h"
#include <string>
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
    void set(const std::string &s, TIME_mks block_timestamp)
    {
        data->container=s;
        // data->setDirty(roll, block_timestamp);
    }
    void touch(TIME_mks t)
    {
        act->update_time(t,roll);
    }
    std::string get()
    {
        return data->container;
    }
    TTL_sec ttl()
    {
        return to_sec(act->ttl);   // внутри мкс, наружу секунды
    }
    void update_time(TIME_mks t, Rollback* r)
    {
        act->update_time(t,r);
    }
    // void setDirty(Rollback* r, TIME_mks block_timestamp)
    // {
    //     data->setDirty(r,block_timestamp);
    // }


};
struct CNodeConst_prod: public CNodeConst
{
    CNodeConst_prod(const REF_getter<const bc_granule>& _data, const REF_getter<const bc_activity>& _act): data(_data),act(_act){}
    REF_getter<const bc_granule> data=nullptr;
    REF_getter<const bc_activity> act=nullptr;
    std::string get() const override
    {
        return data->container;
    }
    TTL_sec ttl() const override
    {
        return to_sec(act->ttl);   // внутри мкс, наружу секунды
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
                                             TIME_mks block_timestamp, TTL_sec ttl,
                                             Rollback* roll) override {
        MUTEX_INSPECTOR;                                            
        {
            auto d = cells::getContractDataNoCreateConst(root,db.get(),contract, key);
            if (d.valid()) return std::string("create: leaf already exists");
        }
        auto dd = cells::getContractDataOrCreate(root,db.get(),contract, key, ttl, roll, block_timestamp);
        if (!dd.valid()) return std::string("create failed");
        {  dd->container = value; }
        {
            auto actd = cells::getActivityOrCreate(root, db.get(), dd->getDbId(),{},ttl,roll,block_timestamp);
            if(!actd.valid())
                throw CommonError("if(!act.valid())");
            actd->update_time(TIME_mks{block_timestamp}, roll);
            return std::nullopt;
        }
    }

    std::optional<std::string> setGranule(Cellable* root,const CONTRACT_id& contract, const CONTRACT_DATA_id& key,
                                        const std::string& value,
                                        TIME_mks block_timestamp, 
                                        Rollback* roll) override {
        MUTEX_INSPECTOR;                                            
        auto d = cells::getContractDataNoCreate(root,db.get(),contract, key, roll, block_timestamp);
        if (!d.valid()) return std::string("granule not exists");
        d->container = value;   // <-- ОБНОВИТЬ ЗНАЧЕНИЕ
        auto actd=cells::getActivityNoCreate(root,db.get(),d->getDbId(), roll, block_timestamp);
        actd->update_time(TIME_mks{block_timestamp},roll);

        return std::nullopt;
    }
    std::optional<std::string> addContractBalance(Cellable* root, const CONTRACT_id& c, int64_t delta, Rollback* roll, TIME_mks block_timestamp) override {
        MUTEX_INSPECTOR;                                            
        auto b = cells::getContractBalanceOrCreate(root, db.get(), c, roll, block_timestamp);
        if (!b.valid()) return std::string("balance create failed");
        b->addAmount(delta);   // ← ОБЯЗАТЕЛЬНО (diff + calc_tree_hash)
        DBG(logErr2("addContractBalance: contract=%s delta=%lld amount=%lld", c.container.c_str(), (long long)delta, (long long)b->getAmount()));
        return std::nullopt;
    }
    bool getContractBalance(Cellable* root, const CONTRACT_id& c, int64_t& out) override {
        MUTEX_INSPECTOR;                                            
        auto b = cells::getContractBalanceNoCreateConst(root, db.get(), c);
        if (!b.valid()) { logErr2("getContractBalance: %s NOT FOUND", c.container.c_str()); return false; }
        out = b->getAmount();
        DBG(logErr2("getContractBalance: contract=%s amount=%lld", c.container.c_str(), (long long)out));
        return true;
    }
    std::optional<std::string> deleteGranule(Cellable* root, const CONTRACT_id& contract, const CONTRACT_DATA_id& key,
                                             Rollback* roll,
                                             std::set<REF_getter<data_base>>* deleted_granules) override {
        MUTEX_INSPECTOR;                                            
        auto d = cells::getContractDataNoCreateConst(root,db.get(), contract, key);
        if (!d.valid()) return std::string("granule not exists");
        deleted_granules->insert(d.get());
        return std::nullopt;
    }
    int64_t getGranuleTTL(Cellable* root, const CONTRACT_id& contract, const CONTRACT_DATA_id& key) override {
        MUTEX_INSPECTOR;                                            
        auto d = cells::getContractDataNoCreateConst(root, db.get(), contract, key);
        if (!d.valid()) return 0;
        auto actd=cells::getActivityNoCreateConst(root,db.get(),d->getDbId());
        if (!actd.valid()) return 0;   // активность удалена/GC — ttl неизвестен
        return to_sec(actd->ttl).v;   // наружу секунды
    }
    REF_getter<CNode> createCNode(Cellable* root,const CONTRACT_id& c, const CONTRACT_DATA_id& cd,
                                                     TIME_mks block_timestamp, TTL_sec ttl,
                                                     Rollback* r) override
    {
        auto dbid= blake2b_hash(cells::getContractDataKey(c,cd));
        auto d=cells::getContractDataNoCreate(root,db.get(),c,cd,r,block_timestamp);
        if(d.valid())
            throw CommonError("if(d.valid())#4763@#"); /// error data already exists
        REF_getter<CNode_prod> ret=new CNode_prod;
        ret->data=cells::getContractDataOrCreate(root,db.get(),c,cd,ttl,r,block_timestamp);
        if(!ret->data.valid())
            throw CommonError("if(!ret->data.valid()) @@@#$$");
        ret->act=cells::getActivityOrCreate(root,db.get(),dbid.container,{},ttl,r,block_timestamp);
        if(!ret->act.valid())
            throw CommonError("if(!act.valid()) @#886");

        return ret.get();
    }
    REF_getter<CNode> getCNode(Cellable* root,const CONTRACT_id& c, const CONTRACT_DATA_id& cd, TIME_mks t, Rollback* r) override
    {
        auto dbid= blake2b_hash(cells::getContractDataKey(c,cd));
        REF_getter<CNode_prod> ret=new CNode_prod;
        ret->data=cells::getContractDataNoCreate(root,db.get(),c,cd,r,t);
        if(!ret->data.valid())
            return nullptr; /// error not already exists
        ret->act=cells::getActivityNoCreate(root,db.get(),dbid.container,r,t);
        if(!ret->act.valid())
            throw CommonError("if(!act.valid()) @#886");

        return ret.get();
    }
    REF_getter<const CNodeConst> getCNodeConst(Cellable* root,const CONTRACT_id& c, const CONTRACT_DATA_id& cd) override
    {
        auto dbid= blake2b_hash(cells::getContractDataKey(c,cd));
        auto data = cells::getContractDataNoCreateConst(root,db.get(),c,cd);
        auto act  = cells::getActivityNoCreateConst(root,db.get(),dbid.container);
        if (!data.valid() || !act.valid())
            return nullptr;   // гранула/активность удалена (тумбстоун) — «не найдено»
        REF_getter<const CNodeConst_prod> ret=new CNodeConst_prod(data,act);
        // ret->data=cells::getContractDataNoCreateConst(root,db.get(),c,cd);
        // if(!ret->data.valid())
        //     return nullptr; /// error not already exists
        // ret->act=cells::getActivityNoCreateConst(root,db.get(),dbid.container,r,t);
        // if(!ret->act.valid())
        //     throw CommonError("if(!act.valid()) @#886");

        return ret.get();
    }
    void update_contract_time(Cellable* root,const CONTRACT_id& c, TIME_mks t, Rollback* r) override
    {
        auto k=cells::getContractKey(c);
        auto act=cells::getActivityNoCreate(root,db.get(),blake2b_hash(k).container,r,t);
        if(!act.valid())
            throw CommonError("if(!act.valid())");
        act->update_time(t,r);
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