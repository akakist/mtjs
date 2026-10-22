#pragma once
#include "REF.h"
#include <string>
#include "CONTRACT_id.h"
struct ContractSourceIO : public Refcountable {
    ContractSourceIO() : Refcountable("ContractSourceIO") {}
    virtual ~ContractSourceIO() = default;
    virtual bool load(const CONTRACT_id& name, std::string& src) = 0;
};
