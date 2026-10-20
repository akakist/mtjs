#include "globals.h"
#include <quickjsi_i.h>
#include <iostream>
#include <cstdio>

void print_exception(JSContext* ctx) {
    JSValue exc = JSI_GetException(ctx);
    const char* s = JSI_ToCString(ctx, exc);
    if (s) { std::cerr << "Exception: " << s << std::endl; JSI_FreeCString(ctx, s); }
    JSI_FreeValue(ctx, exc);
}

void delete_prop_str(JSContext* ctx, JSValueConst obj, const char* name) {
    JSAtom atom = JSI_NewAtom(ctx, name);
    if (atom != JSI_ATOM_NULL) {
        JSI_DeleteProperty(ctx, obj, atom, 0);
        JSI_FreeAtom(ctx, atom);
    }
}

std::shared_ptr<ContractContext> load_contract(
    const std::string& name, const std::string& path)
{
    auto it = contract_cache.find(name);
    if (it != contract_cache.end()) return it->second;

    auto cc = std::make_shared<ContractContext>();
    cc->name = name;
    cc->path = path;

    if (!cc->create()) {
        std::cerr << "cannot create context for " << name << std::endl;
        return nullptr;
    }

    register_api(cc->ctx);

    // Удаляем опасные объекты
    JSValue global = JSI_GetGlobalObject(cc->ctx);
    const char* banned[] = {
        "Math","Date","RegExp","JSON","Proxy","Promise","Map","Set",
        "WeakRef","FinalizationRegistry","Atomics","SharedArrayBuffer"
    };
    for (auto b : banned) delete_prop_str(cc->ctx, global, b);
    JSI_FreeValue(cc->ctx, global);

    // Читаем файл
    std::string src;
    {
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) { std::cerr << "cannot open " << path << std::endl; return nullptr; }
        fseek(f, 0, SEEK_END); long size = ftell(f); fseek(f, 0, SEEK_SET);
        src.resize(size);
        auto sz = fread(&src[0], 1, size, f);
        fclose(f);
        if (sz != (size_t)size) return nullptr;
    }

    JSValue compiled = JSI_Eval(cc->ctx, src.data(), src.size(), path.c_str(),
        JSI_EVAL_FLAG_STRICT | JSI_EVAL_TYPE_GLOBAL | JSI_EVAL_FLAG_COMPILE_ONLY);
    if (JSI_IsException(compiled)) { print_exception(cc->ctx); return nullptr; }

    JSValue result = JSI_EvalFunction(cc->ctx, compiled);
    if (JSI_IsException(result)) {
        print_exception(cc->ctx);
        JSI_FreeValue(cc->ctx, result);
        return nullptr;
    }
    JSI_FreeValue(cc->ctx, result);

    contract_cache[name] = cc;
    return cc;
}