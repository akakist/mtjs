#include "globals.h"
#include <quickjs_i.h>
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
    if (JS_GetOwnPropertyNames(ctx, &props, &count, obj,
            JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) != 0) return;

    for (uint32_t i = 0; i < count; ++i) {
        const char* k = JS_AtomToCString(ctx, props[i].atom);
        if (k) {
            JSValue v = JS_GetProperty(ctx, obj, props[i].atom);
            out[k] = JSValueGuard(ctx, JS_DupValue(ctx, v));
            JS_FreeValue(ctx, v);
            JS_FreeCString(ctx, k);
        }
        JS_FreeAtom(ctx, props[i].atom);
    }
    js_free(ctx, props);
}

static std::string read_string_prop(JSContext* ctx, JSValueConst obj, const char* name) {
    JSValue v = JS_GetPropertyStr(ctx, obj, name);
    std::string r;
    if (JS_IsString(v)) {
        const char* s = JS_ToCString(ctx, v);
        if (s) { r = s; JS_FreeCString(ctx, s); }
    }
    JS_FreeValue(ctx, v);
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

    JSValue compiled = JS_Eval(ctx, src.data(), src.size(), path.c_str(),
        JS_EVAL_FLAG_STRICT | JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_COMPILE_ONLY);
    if (JS_IsException(compiled)) { print_exception(ctx); return false; }

    JSValue result = JS_EvalFunction(ctx, compiled);
    if (JS_IsException(result)) { print_exception(ctx); JS_FreeValue(ctx, result); return false; }

    if (!JS_IsObject(result)) {
        std::cerr << path << ": result is not an object" << std::endl;
        JS_FreeValue(ctx, result);
        return false;
    }

    // initial
    JSValue initial_val = JS_GetPropertyStr(ctx, result, "initial");
    if (JS_IsObject(initial_val)) read_object_map(ctx, initial_val, out.initial);
    JS_FreeValue(ctx, initial_val);

    // transactions
    JSValue tx_val = JS_GetPropertyStr(ctx, result, "transactions");
    if (JS_IsArray(ctx, tx_val)) {
        JSValue lv = JS_GetPropertyStr(ctx, tx_val, "length");
        int64_t len = 0; JS_ToInt64(ctx, &len, lv); JS_FreeValue(ctx, lv);

        for (int64_t i = 0; i < len; ++i) {
            JSValue item = JS_GetPropertyUint32(ctx, tx_val, (uint32_t)i);
            if (!JS_IsObject(item)) { JS_FreeValue(ctx, item); continue; }

            Tx tx;
            tx.contract = read_string_prop(ctx, item, "contract");
            tx.method   = read_string_prop(ctx, item, "method");

            JSValue pv = JS_GetPropertyStr(ctx, item, "params");
            tx.params = JSValueGuard(ctx,
                JS_IsUndefined(pv) ? JS_NewObject(ctx) : JS_DupValue(ctx, pv));
            JS_FreeValue(ctx, pv);

            JSValue ev = JS_GetPropertyStr(ctx, item, "expect_error");
            if (!JS_IsUndefined(ev)) tx.expect_error = JS_ToBool(ctx, ev);
            JS_FreeValue(ctx, ev);

            JSValue rv = JS_GetPropertyStr(ctx, item, "expect_result");
            if (!JS_IsUndefined(rv)) tx.expect_result = JSValueGuard(ctx, JS_DupValue(ctx, rv));
            JS_FreeValue(ctx, rv);

            out.transactions.push_back(std::move(tx));
            JS_FreeValue(ctx, item);
        }
    }
    JS_FreeValue(ctx, tx_val);

    // expected
    JSValue expected_val = JS_GetPropertyStr(ctx, result, "expected");
    if (JS_IsObject(expected_val)) read_object_map(ctx, expected_val, out.expected);
    JS_FreeValue(ctx, expected_val);

    JS_FreeValue(ctx, result);
    return true;
}