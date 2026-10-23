#pragma once
#include "jsi/api/GranuleIO.h"
#include <string>
#include "jsi/api/EventSink.h"
#include "jsi/api/Clock.h"
#include "jsi/api/ContractSourceIO.h"
#include "IDatabase.h"
#include "REF.h"
#include "contract_constants.h"

struct NodeGranuleIO : GranuleIO {
    NodeGranuleIO(const REF_getter<IDatabase> &_db) : db(_db) {}

    REF_getter<IDatabase> db;

    bool get(const CONTRACT_id& contract,  const CONTRACT_DATA_id& key, std::string& out) override {
        auto d=db->getContractDataNoCreateConst(contract,key);
        if(!d.valid()) return false;
        M_LOCK(d->parent->mx);
        out=d->container;
        return true;
    }
    bool exists(const CONTRACT_id& contract,  const CONTRACT_DATA_id& key) override {
        auto d=db->getContractDataNoCreateConst(contract,key);
        return d.valid();
    }
    std::optional<std::string> create(const CONTRACT_id& contract,  const CONTRACT_DATA_id& key, const std::string& value,
                int64_t create_time, int64_t ttl, Rollback* roll) override {
        auto d=db->getContractDataNoCreateConst(contract,key);
        if(d.valid()) return std::string("create: leaf already exists");
        auto dd=db->getContractDataOrCreate(contract,key,roll,ttl, create_time);
        if(!dd.valid()) return std::string("create failed");
        {
            M_LOCK(dd->parent->mx);
            dd->container=value;
        }
        return std::nullopt;
    }
    std::optional<std::string> set(const CONTRACT_id& contract,  const CONTRACT_DATA_id& key, const std::string& value, Rollback* roll) override {
        auto d=db->getContractDataNoCreate(contract,key);
        if(!d.valid()) return std::string("granule not exists");
        d->setDirty(roll);
        {
            M_LOCK(d->parent->mx);
            d->container=value;
        }
        return std::nullopt;
    }
};

struct NodeEventSink : EventSink
{

    NodeEventSink() : EventSink() {}
    void emit(const CONTRACT_id& contract, const std::string& name,
              const std::string& value) override {
        (void)contract; (void)name; (void)value;
        // TODO: блок-очередь событий, flush в БД/лог
    }
};

struct NodeClock : Clock
{
    NodeClock() : Clock() {}
    int64_t current_time() override { return 0; }   // TODO: реальное время блока
    int64_t current_block_height() override { return 0; }   // TODO
};

struct NodeContractSourceIO : ContractSourceIO
{
    REF_getter<IDatabase> db=nullptr;

    NodeContractSourceIO(const REF_getter<IDatabase> &_db) : db(_db) {}

    bool load(const CONTRACT_id& name, std::string& src) override
    {
        auto cc=db->getContractNoCreateConst(name);
        if(!cc.valid())
        {
            logErr2("if(!cc.valid()) %s",name.container.c_str());
            return false;
        }
        src=cc->src;
        return true;
    }
};

