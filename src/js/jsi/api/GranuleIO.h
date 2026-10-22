#pragma once
#include <string>
#include <optional>
#include "REF.h"
#include "CONTRACT_id.h"
#include "CONTRACT_DATA_id.h"
#include "Rollback.h"
struct GranuleIO : public Refcountable {
    GranuleIO() : Refcountable("GranuleIO") {}
    virtual ~GranuleIO() = default;
    virtual bool get(const CONTRACT_id& c, const CONTRACT_DATA_id& cd, std::string& out) = 0;
    virtual bool exists(const CONTRACT_id& c, const CONTRACT_DATA_id& cd) = 0;
    virtual std::optional<std::string> create(const CONTRACT_id& c, const CONTRACT_DATA_id& cd, const std::string& value,
                        int64_t create_time, int64_t ttl, Rollback* r) = 0;
    virtual std::optional<std::string> set(const CONTRACT_id& c, const CONTRACT_DATA_id& cd, const std::string& value, Rollback* roll) = 0;
};
