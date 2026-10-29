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

    JSIValue call_contract_method(const std::string& method_name,
                             const nlohmann::json& params,
                             IMsgSource* msg_source, bool is_view,
                             const REF_getter<CallStack>& call_stack,
                             const REF_getter<ContractHost>& host);
};



