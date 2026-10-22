#include "quickjs_i.h"
#include <iostream>
#include "ContractHost.h"
#include "ContractContext.h"
#include "convert.h"
#include "contract_api.h"

REF_getter<ContractContext> ContractHost::load_contract(const CONTRACT_id& name) {
    auto it = contract_cache.find(name);
    if (it != contract_cache.end()) return it->second;

    std::string src;
    if (!source->load(name, src)) {
        return nullptr;
    }

    REF_getter<ContractContext> cc = new ContractContext();
    cc->name = name;

    if (!cc->create__()) {
        std::cerr << "cannot create context for " << name.container << std::endl;
        return nullptr;
    }

    register_api(cc->ctx);

    JSIValue global = JSI_GetGlobalObject(cc->ctx);
    const char* banned[] = {
        "Math","Date","RegExp","JSON","Proxy","Promise","Map","Set",
        "WeakRef","FinalizationRegistry","Atomics","SharedArrayBuffer"
    };
    for (auto b : banned) delete_prop_str(cc->ctx, global, b);
    JSI_FreeValue(cc->ctx, global);

    JSIValue compiled = JSI_Eval(cc->ctx, src.data(), src.size(), name.container.c_str(),
        JSI_EVAL_FLAG_STRICT | JSI_EVAL_TYPE_GLOBAL | JSI_EVAL_FLAG_COMPILE_ONLY);
    if (JSI_IsException(compiled)) { print_exception(cc->ctx); return nullptr; }

    JSIValue result = JSI_EvalFunction(cc->ctx, compiled);
    if (JSI_IsException(result)) {
        print_exception(cc->ctx);
        JSI_FreeValue(cc->ctx, result);
        return nullptr;
    }
    JSI_FreeValue(cc->ctx, result);

    contract_cache[name] = cc;
    return cc;
}

REF_getter<ContractContext> ContractHost::get_contract(const CONTRACT_id& name) {
    auto it = contract_cache.find(name);
    if (it != contract_cache.end()) return it->second;
    return load_contract(name);
}



