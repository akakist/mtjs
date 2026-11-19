#pragma once
#include <string>
#include <optional>
#include "REF.h"
#include "CONTRACT_id.h"
#include "CONTRACT_DATA_id.h"
#include "Rollback.h"
#include "db_to_save.h"
#include "time_types.h"
#include "cellable.h"

struct CNode: public Refcountable
{
    CNode(): Refcountable("CNode"){}
    virtual void set(const std::string &s, TIME_mks block_timestamp) =0;
    virtual std::string get() =0;
    virtual void touch(TIME_mks t)=0;
    virtual TTL_sec ttl()=0;
    virtual void update_time(TIME_mks t, Rollback* r)=0;
    virtual void setDirty(Rollback* r, TIME_mks block_timestamp)=0;
    virtual ~CNode() = default;
};
struct GranuleIO : public Refcountable {
    GranuleIO() : Refcountable("GranuleIO") {}
    virtual ~GranuleIO() = default;

    virtual bool getGranule(Cellable*, const CONTRACT_id& c, const CONTRACT_DATA_id& cd, std::string& out) = 0;
    virtual bool existsGranule(Cellable*,const CONTRACT_id& c, const CONTRACT_DATA_id& cd) = 0;

    virtual std::optional<std::string> createGranule(Cellable*,const CONTRACT_id& c, const CONTRACT_DATA_id& cd,
                                                     const std::string& value,
                                                     TIME_mks create_time, TTL_sec ttl,
                                                     Rollback* r) = 0;
    virtual std::optional<std::string> setGranule(Cellable*,const CONTRACT_id& c, const CONTRACT_DATA_id& cd,
                                                  const std::string& value,
                                                  TIME_mks now,
                                                  Rollback* roll) = 0;

    virtual REF_getter<CNode> createCNode(Cellable*,const CONTRACT_id& c, const CONTRACT_DATA_id& cd,
                                                     TIME_mks create_time, TTL_sec ttl,
                                                     Rollback* r) = 0;
    virtual REF_getter<CNode> getCNode(Cellable*,const CONTRACT_id& c, const CONTRACT_DATA_id& cd) = 0;
                                                         

    virtual bool getContractBalance(Cellable*,const CONTRACT_id& c, int64_t& out) = 0;
    virtual std::optional<std::string> addContractBalance(Cellable*,const CONTRACT_id& c, int64_t delta, Rollback* roll, TIME_mks block_timestamp) = 0;

    virtual std::optional<std::string> deleteGranule(Cellable*,const CONTRACT_id& contract, const CONTRACT_DATA_id& key,
                                                     Rollback* roll,
                                                     std::set<REF_getter<data_base>>* deleted_granules) = 0;

    virtual int64_t getGranuleTTL(Cellable*,const CONTRACT_id& c, const CONTRACT_DATA_id& cd) = 0;
    virtual void update_contract_time(Cellable*,const CONTRACT_id& c, TIME_mks t, Rollback* roll) =0;
};