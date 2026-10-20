#pragma once
#include <quickjsi_i.h>
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
        if (ctx) { JSI_FreeContext(ctx); ctx = nullptr; }
        if (rt) { JSI_FreeRuntime(rt); rt = nullptr; }
    }

    bool create() {
        rt = JSI_NewRuntime();
        if (!rt) return false;
        JSI_SetMemoryLimit(rt, 64 * 1024 * 1024);
        JSI_SetMaxStackSize(rt, 1024 * 1024);

        ctx = JSI_NewContext(rt);
        if (!ctx) { JSI_FreeRuntime(rt); rt = nullptr; return false; }

        JSI_SetContextOpaque(ctx, this);
        return true;
    }
};
