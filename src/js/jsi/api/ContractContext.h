#pragma once
#include "REF.h"
#include "quickjs_i.h"
#include <map>
#include <string>
#include <nlohmann/json.hpp>
#include "contract_constants.h"
#include "jsiValueGuard.h"
#include "jsiMsg.h"
#include "CONTRACT_id.h"
#include "IMsgSource.h"
#include "Rollback.h"
#include "FeesSnapshot.h"
#include "db_to_save.h"
#include "cellable.h"
struct CallStack;
struct ContractHost;

struct ContractContext : public Refcountable {
    JSIRuntime* rt = nullptr;
    JSIContext* ctx = nullptr;
    CONTRACT_id name;
    std::map<std::string, JSIValueGuard> methods;
    std::map<std::string, JSIValueGuard> views;

    ContractContext() : Refcountable("ContractContext") {}
    ContractContext(const ContractContext&) = delete;
    ContractContext& operator=(const ContractContext&) = delete;
    // REF_getter<Cellable> root;

    void clear()
    {
        // root=nullptr;
    }
    ~ContractContext() {
        methods.clear(); views.clear();
        if (ctx) { JSI_FreeContext(ctx); ctx = nullptr; }
        if (rt) { JSI_FreeRuntime(rt); rt = nullptr; }
    }

    bool create__() {
        rt = JSI_NewRuntime();
        if (!rt) return false;
        JSI_SetMemoryLimit(rt, 64 * 1024 * 1024);
        JSI_SetMaxStackSize(rt, MAX_CONTRACT_STACK);
        ctx = JSI_NewContext(rt);
        if (!ctx) { JSI_FreeRuntime(rt); rt = nullptr; return false; }
        JSI_SetContextOpaque(ctx, this);
        return true;
    }

    // method: msg есть, газ есть, roll есть
    JSIValue call_contract_method(Cellable* root, const std::string& method_name,
                             const nlohmann::json& params,
                             IMsgSource* msg_source,
                             CallStack* call_stack,
                              ContractHost* host, Rollback* roll,
                             const CONTRACT_id& caller, FeesSnapshot* fees,
                             std::set<REF_getter<data_base>> * deleted_granules);

    // view: msg НЕТ, газа НЕТ, roll НЕТ (read-only)
    // view: msg НЕТ, газа НЕТ, roll НЕТ (read-only). timeout_sec — лимит времени.
    JSIValue call_contract_view(Cellable* root,const std::string& view_name,
                             const nlohmann::json& params,
                             CallStack* call_stack,
                              ContractHost* host,
                             int64_t timeout_sec = 5);
};


