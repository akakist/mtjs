#pragma once
#include <quickjs_i.h>
#include <string>
#include <map>
#include <memory>
#include <set>
#include "jsValueGuard.h"

struct ContractContext {
    JSRuntime* rt = nullptr;
    JSContext* ctx = nullptr;
    std::string name;
    std::string path;

    std::map<std::string, JSValueGuard> methods;
    std::map<std::string, JSValueGuard> views;

    ContractContext() = default;
    ContractContext(const ContractContext&) = delete;
    ContractContext& operator=(const ContractContext&) = delete;

    ~ContractContext() {
        methods.clear();
        views.clear();
        if (ctx) { JS_FreeContext(ctx); ctx = nullptr; }
        if (rt) { JS_FreeRuntime(rt); rt = nullptr; }
    }

    bool create() {
        rt = JS_NewRuntime();
        if (!rt) return false;
        JS_SetMemoryLimit(rt, 64 * 1024 * 1024);
        JS_SetMaxStackSize(rt, 1024 * 1024);

        ctx = JS_NewContext(rt);
        if (!ctx) { JS_FreeRuntime(rt); rt = nullptr; return false; }

        JS_SetContextOpaque(ctx, this);
        return true;
    }
};
