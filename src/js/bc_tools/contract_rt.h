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
    std::map<std::string,JSValueGuard> mutable_methods;
    std::map<std::string,JSValueGuard> immutable_methods;
    std::string src;
    ADDRESS_id owner;
};
struct execute_context: public Refcountable
{
    REF_getter<contract_rt> contract;
    std::string tx_hash_bin;
    std::string tx_sender_bin;
    uint64_t nonce;
    uint64_t tx_timestamp;
    uint64_t txEpoch;
    std::string contractAddress;
    std::string ownerAddress;


};