#pragma once
#include "quickjs.h"
#include "jsValueGuard.h"
#include <map>
#include <string>
#include "REF.h"
struct contract_rt: public Refcountable
{
    contract_rt():Refcountable("contract_rt"){}
    JSContext *ctx;
    JSRuntime *rt;
    std::map<std::string,JSValueGuard> methods;
    std::string src;
    ADDRESS_id owner;

};