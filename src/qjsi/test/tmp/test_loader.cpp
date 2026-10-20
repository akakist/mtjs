#include "globals.h"
#include <quickjsi_i.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstdio>

struct Tx {
    std::string contract;
    std::string method;
    JSValueGuard params;
    bool expect_error = false;
    JSValueGuard expect_result;
};

struct TestCase {
    std::map<std::string, JSValueGuard> initial;
    std::vector<Tx> transactions;
    std::map<std::string, JSValueGuard> expected;
};

// Вспомогательная функция: читает объект JS в std::map<string, JSValueGuard>
static void read_object_map(JSContext* ctx, JSValueConst obj,
                            std::map<std::string, JSValueGuard>& out)
{
    JSPropertyEnum* props = nullptr;
    uint32_t count = 0;
    if (JSI_GetOwnPropertyNames(ctx, &props, &count, obj,
            JSI_GPN_STRING_MASK | JSI_GPN_ENUM_ONLY) != 0) return;

    for (uint32_t i = 0; i < count; ++i) {
        const char* k = JSI_AtomToCString(ctx, props[i].atom);
        if (k) {
            JSValue v = JSI_GetProperty(ctx, obj, props[i].atom);
            out[k] = JSValueGuard(ctx, JSI_DupValue(ctx, v));
            JSI_FreeValue(ctx, v);
            JSI_FreeCString(ctx, k);
        }
        JSI_FreeAtom(ctx, props[i].atom);
    }
    jsi_free(ctx, props);
}

static std::string read_string_prop(JSContext* ctx, JSValueConst obj, const char* name) {
    JSValue v = JSI_GetPropertyStr(ctx, obj, name);
    std::string r;
    if (JSI_IsString(v)) {
        const char* s = JSI_ToCString(ctx, v);
        if (s) { r = s; JSI_FreeCString(ctx, s); }
    }
    JSI_FreeValue(ctx, v);
    return r;
}

bool load_test(JSContext* ctx, const std::string& path, TestCase& out) {
    std::string src;
    {
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) { std::cerr << "cannot open " << path << std::endl; return false; }
        fseek(f, 0, SEEK_END); long size = ftell(f); fseek(f, 0, SEEK_SET);
        src.resize(size);
        auto sz = fread(&src[0], 1, size, f);
        fclose(f);
        if (sz != (size_t)size) return false;
    }

    JSValue compiled = JSI_Eval(ctx, src.data(), src.size(), path.c_str(),
        JSI_EVAL_FLAG_STRICT | JSI_EVAL_TYPE_GLOBAL | JSI_EVAL_FLAG_COMPILE_ONLY);
    if (JSI_IsException(compiled)) { print_exception(ctx); return false; }

    JSValue result = JSI_EvalFunction(ctx, compiled);
    if (JSI_IsException(result)) { print_exception(ctx); JSI_FreeValue(ctx, result); return false; }

    if (!JSI_IsObject(result)) {
        std::cerr << path << ": result is not an object" << std::endl;
        JSI_FreeValue(ctx, result);
        return false;
    }

    // initial
    JSValue initial_val = JSI_GetPropertyStr(ctx, result, "initial");
    if (JSI_IsObject(initial_val)) read_object_map(ctx, initial_val, out.initial);
    JSI_FreeValue(ctx, initial_val);

    // transactions
    JSValue tx_val = JSI_GetPropertyStr(ctx, result, "transactions");
    if (JSI_IsArray(ctx, tx_val)) {
        JSValue lv = JSI_GetPropertyStr(ctx, tx_val, "length");
        int64_t len = 0; JSI_ToInt64(ctx, &len, lv); JSI_FreeValue(ctx, lv);

        for (int64_t i = 0; i < len; ++i) {
            JSValue item = JSI_GetPropertyUint32(ctx, tx_val, (uint32_t)i);
            if (!JSI_IsObject(item)) { JSI_FreeValue(ctx, item); continue; }

            Tx tx;
            tx.contract = read_string_prop(ctx, item, "contract");
            tx.method   = read_string_prop(ctx, item, "method");

            JSValue pv = JSI_GetPropertyStr(ctx, item, "params");
            tx.params = JSValueGuard(ctx,
                JSI_IsUndefined(pv) ? JSI_NewObject(ctx) : JSI_DupValue(ctx, pv));
            JSI_FreeValue(ctx, pv);

            JSValue ev = JSI_GetPropertyStr(ctx, item, "expect_error");
            if (!JSI_IsUndefined(ev)) tx.expect_error = JSI_ToBool(ctx, ev);
            JSI_FreeValue(ctx, ev);

            JSValue rv = JSI_GetPropertyStr(ctx, item, "expect_result");
            if (!JSI_IsUndefined(rv)) tx.expect_result = JSValueGuard(ctx, JSI_DupValue(ctx, rv));
            JSI_FreeValue(ctx, rv);

            out.transactions.push_back(std::move(tx));
            JSI_FreeValue(ctx, item);
        }
    }
    JSI_FreeValue(ctx, tx_val);

    // expected
    JSValue expected_val = JSI_GetPropertyStr(ctx, result, "expected");
    if (JSI_IsObject(expected_val)) read_object_map(ctx, expected_val, out.expected);
    JSI_FreeValue(ctx, expected_val);

    JSI_FreeValue(ctx, result);
    return true;
}