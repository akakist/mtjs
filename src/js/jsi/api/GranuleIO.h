#pragma once
#include <string>
#include <optional>
#include "REF.h"
#include "CONTRACT_id.h"
#include "CONTRACT_DATA_id.h"
#include "Rollback.h"
#include "db_to_save.h"
#include "cellable.h"

struct GranuleIO : public Refcountable {
    GranuleIO() : Refcountable("GranuleIO") {}
    virtual ~GranuleIO() = default;

    virtual bool getGranule(const CONTRACT_id& c, const CONTRACT_DATA_id& cd, std::string& out) = 0;
    virtual bool existsGranule(const CONTRACT_id& c, const CONTRACT_DATA_id& cd) = 0;

    virtual std::optional<std::string> createGranule(const CONTRACT_id& c, const CONTRACT_DATA_id& cd,
                                                     const std::string& value,
                                                     int64_t create_time, int64_t ttl,
                                                     Rollback* r) = 0;
    virtual std::optional<std::string> setGranule(const CONTRACT_id& c, const CONTRACT_DATA_id& cd,
                                                  const std::string& value,
                                                  int64_t now, int64_t ttl,
                                                  Rollback* roll) = 0;

    virtual bool getContractBalance(const CONTRACT_id& c, int64_t& out) = 0;
    virtual std::optional<std::string> addContractBalance(const CONTRACT_id& c, int64_t delta, Rollback* roll) = 0;

    virtual std::optional<std::string> deleteGranule(const CONTRACT_id& contract, const CONTRACT_DATA_id& key,
                                                     Rollback* roll,
                                                     std::set<REF_getter<data_base>>* deleted_granules) = 0;

    virtual int64_t getGranuleTTL(const CONTRACT_id& c, const CONTRACT_DATA_id& cd) = 0;
};