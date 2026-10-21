#pragma once
#include "REF.h"
#include <string>
struct ContractSourceIO : public Refcountable {
    ContractSourceIO() : Refcountable("ContractSourceIO") {}
    virtual ~ContractSourceIO() = default;
    virtual bool load(const std::string& name, std::string& src) = 0;
};
