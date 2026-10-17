#include "globals.h"
#include "jsscope.h"
#include <quickjs_i.h>
#include <iostream>

// ---- serialize_value ----
std::string serialize_value(JSContext* ctx, JSValueConst val) {
    if (JS_IsUndefined(val)) return "undefined";
    if (JS_IsNull(val)) return "null";
    if (JS_IsBool(val)) return JS_ToBool(ctx, val) ? "true" : "false";

    if (JS_IsNumber(val)) {
        int64_t n;
        if (JS_ToInt64(ctx, &n, val) == 0) return std::to_string(n);
        return "<float>";
    }

    if (JS_IsString(val)) {
        size_t len = 0;
        const char* s = JS_ToCStringLen(ctx, &len, val);
        if (!s) return "<string-error>";
        std::string r = "\"";
        r.append(s, len);
        r += "\"";
        JS_FreeCString(ctx, s);
        return r;
    }

    if (JS_IsArray(ctx, val)) {
        std::string r = "[";
        JSValue lv = JS_GetPropertyStr(ctx, val, "length");
        int64_t len = 0; JS_ToInt64(ctx, &len, lv); JS_FreeValue(ctx, lv);
        for (int64_t i = 0; i < len; ++i) {
            if (i) r += ",";
            JSValue item = JS_GetPropertyUint32(ctx, val, (uint32_t)i);
            r += serialize_value(ctx, item);
            JS_FreeValue(ctx, item);
        }
        return r + "]";
    }

    if (JS_IsObject(val)) {
        JSPropertyEnum* props = nullptr;
        uint32_t count = 0;
        std::vector<std::string> keys;
        if (JS_GetOwnPropertyNames(ctx, &props, &count, val,
                JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) == 0) {
            for (uint32_t i = 0; i < count; ++i) {
                const char* k = JS_AtomToCString(ctx, props[i].atom);
                if (k) { keys.push_back(k); JS_FreeCString(ctx, k); }
                JS_FreeAtom(ctx, props[i].atom);
            }
            js_free(ctx, props);
        }
        std::sort(keys.begin(), keys.end());
        std::string r = "{";
        bool first = true;
        for (auto& k : keys) {
            if (!first) r += ","; first = false;
            r += "\"" + k + "\":";
            JSValue item = JS_GetPropertyStr(ctx, val, k.c_str());
            r += serialize_value(ctx, item);
            JS_FreeValue(ctx, item);
        }
        return r + "}";
    }

    return "<unknown>";
}

// Конвертация между контекстами через сериализацию
JSValue convert_value(JSContext* from_ctx, JSContext* to_ctx, JSValueConst val) {
    std::string s = serialize_value(from_ctx, val);
    return JS_Eval(to_ctx, s.c_str(), s.size(), "<convert>", JS_EVAL_TYPE_GLOBAL);
}

// ---- registerMethod ----
JSValue js_registerMethod(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 2 || !JS_IsString(argv[0]) || !JS_IsFunction(ctx, argv[1]))
        return JS_ThrowTypeError(ctx, "registerMethod(name, fn)");

    ContractContext* cc = (ContractContext*)JS_GetContextOpaque(ctx);
    if (!cc) return JS_ThrowTypeError(ctx, "no contract context");

    JSScope<20,20> scope(ctx);
    auto name = scope.toStdString(argv[0]);
    if (name.empty()) return JS_EXCEPTION;

    cc->methods[name] = JSValueGuard(ctx, JS_DupValue(ctx, argv[1]));
    return JS_UNDEFINED;
}

// ---- registerView ----
JSValue js_registerView(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 2 || !JS_IsString(argv[0]) || !JS_IsFunction(ctx, argv[1]))
        return JS_ThrowTypeError(ctx, "registerView(name, fn)");

    ContractContext* cc = (ContractContext*)JS_GetContextOpaque(ctx);
    if (!cc) return JS_ThrowTypeError(ctx, "no contract context");

    JSScope<20,20> scope(ctx);
    auto name = scope.toStdString(argv[0]);
    if (name.empty()) return JS_EXCEPTION;

    cc->views[name] = JSValueGuard(ctx, JS_DupValue(ctx, argv[1]));
    return JS_UNDEFINED;
}

// ---- setGranule ----
JSValue js_setGranule(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 2 || !JS_IsString(argv[0]))
        return JS_ThrowTypeError(ctx, "setGranule(key, value)");

    ContractContext* cc = (ContractContext*)JS_GetContextOpaque(ctx);
    if (!cc) return JS_ThrowTypeError(ctx, "no contract context");

    JSScope<20,20> scope(ctx);
    auto key = scope.toStdString(argv[0]);
    if (key.empty()) return JS_EXCEPTION;

    auto it = granules.find(key);
    if (it != granules.end() && it->second.owner_contract != cc->name)
        return JS_ThrowTypeError(ctx, "access denied: granule owned by another contract");

    granules[key] = Granule{
        JSValueGuard(ctx, JS_DupValue(ctx, argv[1])),
        cc->name
    };
    return JS_UNDEFINED;
}

// ---- getGranule ----
JSValue js_getGranule(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 1 || !JS_IsString(argv[0]))
        return JS_ThrowTypeError(ctx, "getGranule(key)");

    ContractContext* cc = (ContractContext*)JS_GetContextOpaque(ctx);
    if (!cc) return JS_ThrowTypeError(ctx, "no contract context");

    JSScope<20,20> scope(ctx);
    auto key = scope.toStdString(argv[0]);
    if (key.empty()) return JS_EXCEPTION;

    auto it = granules.find(key);
    if (it == granules.end()) return JS_UNDEFINED;

    if (it->second.owner_contract != cc->name)
        return JS_ThrowTypeError(ctx, "access denied: granule owned by another contract");

    return JS_DupValue(ctx, it->second.value.get());
}

// ---- callView ----
JSValue js_callView(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 3 || !JS_IsString(argv[0]) || !JS_IsString(argv[1]))
        return JS_ThrowTypeError(ctx, "callView(contract, view, params)");

    JSScope<20,20> scope(ctx);
    auto contract_name = scope.toStdString(argv[0]);
    auto view_name = scope.toStdString(argv[1]);
    if (contract_name.empty() || view_name.empty()) return JS_EXCEPTION;

    // Нельзя вызывать свой же контракт через callView — только напрямую
    ContractContext* self = (ContractContext*)JS_GetContextOpaque(ctx);
    if (self && self->name == contract_name)
        return JS_ThrowTypeError(ctx, "callView to self is not allowed; call directly");

    auto it = contract_cache.find(contract_name);
    if (it == contract_cache.end())
        return JS_ThrowTypeError(ctx, "contract not found: %s", contract_name.c_str());

    auto cc = it->second;
    auto vit = cc->views.find(view_name);
    if (vit == cc->views.end())
        return JS_ThrowTypeError(ctx, "view not found: %s.%s",
            contract_name.c_str(), view_name.c_str());

    JSValue params_in_cc = convert_value(ctx, cc->ctx, argv[2]);
    JSValue result = JS_Call(cc->ctx, vit->second.get(), JS_UNDEFINED, 1, &params_in_cc);
    JS_FreeValue(cc->ctx, params_in_cc);

    if (JS_IsException(result)) {
        // Печатаем исключение в контексте контракта
        JSValue exc = JS_GetException(cc->ctx);
        const char* s = JS_ToCString(cc->ctx, exc);
        if (s) { std::cerr << "callView " << contract_name << "." << view_name
                           << " exception: " << s << std::endl; JS_FreeCString(cc->ctx, s); }
        JS_FreeValue(cc->ctx, exc);
        JS_FreeValue(cc->ctx, result);
        return JS_EXCEPTION;
    }

    JSValue result_in_caller = convert_value(cc->ctx, ctx, result);
    JS_FreeValue(cc->ctx, result);
    return result_in_caller;
}

// ---- register_api ----
void register_api(JSContext* ctx) {
    JSValue global = JS_GetGlobalObject(ctx);

    JS_SetPropertyStr(ctx, global, "registerMethod",
        JS_NewCFunction(ctx, js_registerMethod, "registerMethod", 2));
    JS_SetPropertyStr(ctx, global, "registerView",
        JS_NewCFunction(ctx, js_registerView, "registerView", 2));
    JS_SetPropertyStr(ctx, global, "setGranule",
        JS_NewCFunction(ctx, js_setGranule, "setGranule", 2));
    JS_SetPropertyStr(ctx, global, "getGranule",
        JS_NewCFunction(ctx, js_getGranule, "getGranule", 1));
    JS_SetPropertyStr(ctx, global, "callView",
        JS_NewCFunction(ctx, js_callView, "callView", 3));

    JS_FreeValue(ctx, global);
}
