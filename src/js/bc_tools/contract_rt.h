#pragma once
#include "quickjs.h"
#include "jsValueGuard.h"
#include <map>
#include <string>
#include "REF.h"
#include "ADDRESS_id.h"
#include "CONTRACT_id.h"
#include "root_contract.h"
#include "IDatabase.h"
struct contract_rt: public Refcountable
{
    contract_rt():Refcountable("contract_rt"){}
    JSContext *ctx;
    JSRuntime *rt;
    std::map<std::string,JSValueGuard> methods;
    std::string src;
    ADDRESS_id owner;
    CONTRACT_id name;
    REF_getter<IDatabase> db;
    

};