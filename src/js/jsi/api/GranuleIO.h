#pragma once
#include <string>
#include "REF.h"
struct GranuleIO : public Refcountable {
    GranuleIO() : Refcountable("GranuleIO") {}
    virtual ~GranuleIO() = default;
    virtual bool get(const std::string& key, std::string& out) = 0;
    virtual bool exists(const std::string& key) = 0;
    virtual bool create(const std::string& key, const std::string& value,
                        int64_t create_time, int64_t ttl) = 0;
    virtual bool set(const std::string& key, const std::string& value) = 0;
};
