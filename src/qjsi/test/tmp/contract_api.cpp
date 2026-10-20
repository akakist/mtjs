#include "globals.h"
#include "jsscope.h"
#include <quickjsi_i.h>
#include <iostream>

// ---- serialize_value ----
std::string serialize_value(JSContext* ctx, JSValueConst val) {
    if (JSI_IsUndefined(val)) return "undefined";
    if (JSI_IsNull(val)) return "null";
    if (JSI_IsBool(val)) return JSI_ToBool(ctx, val) ? "true" : "false";

    if (JSI_IsNumber(val)) {
        int64_t n;
        if (JSI_ToInt64(ctx, &n, val) == 0) return std::to_string(n);
        return "<float>";
    }

    if (JSI_IsString(val)) {
        size_t len = 0;
        const char* s = JSI_ToCStringLen(ctx, &len, val);
        if (!s) return "<string-error>";
        std::string r = "\"";
        r.append(s, len);
        r += "\"";
        JSI_FreeCString(ctx, s);
        return r;
    }

    if (JSI_IsArray(ctx, val)) {
        std::string r = "[";
        JSValue lv = JSI_GetPropertyStr(ctx, val, "length");
        int64_t len = 0; JSI_ToInt64(ctx, &len, lv); JSI_FreeValue(ctx, lv);
        for (int64_t i = 0; i < len; ++i) {
            if (i) r += ",";
            JSValue item = JSI_GetPropertyUint32(ctx, val, (uint32_t)i);
            r += serialize_value(ctx, item);
            JSI_FreeValue(ctx, item);
        }
        return r + "]";
    }

    if (JSI_IsObject(val)) {
        JSPropertyEnum* props = nullptr;
        uint32_t count = 0;
        std::vector<std::string> keys;
        if (JSI_GetOwnPropertyNames(ctx, &props, &count, val,
                JSI_GPN_STRING_MASK | JSI_GPN_ENUM_ONLY) == 0) {
            for (uint32_t i = 0; i < count; ++i) {
                const char* k = JSI_AtomToCString(ctx, props[i].atom);
                if (k) { keys.push_back(k); JSI_FreeCString(ctx, k); }
                JSI_FreeAtom(ctx, props[i].atom);
            }
            jsi_free(ctx, props);
        }
        std::sort(keys.begin(), keys.end());
        std::string r = "{";
        bool first = true;
        for (auto& k : keys) {
            if (!first) r += ","; first = false;
            r += "\"" + k + "\":";
            JSValue item = JSI_GetPropertyStr(ctx, val, k.c_str());
            r += serialize_value(ctx, item);
            JSI_FreeValue(ctx, item);
        }
        return r + "}";
    }

    return "<unknown>";
}

// Конвертация между контекстами через сериализацию
JSValue convert_value(JSContext* from_ctx, JSContext* to_ctx, JSValueConst val) {
    std::string s = serialize_value(from_ctx, val);
    return JSI_Eval(to_ctx, s.c_str(), s.size(), "<convert>", JSI_EVAL_TYPE_GLOBAL);
}

// ---- registerMethod ----
JSValue jsi_registerMethod(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 2 || !JSI_IsString(argv[0]) || !JSI_IsFunction(ctx, argv[1]))
        return JSI_ThrowTypeError(ctx, "registerMethod(name, fn)");

    ContractContext* cc = (ContractContext*)JSI_GetContextOpaque(ctx);
    if (!cc) return JSI_ThrowTypeError(ctx, "no contract context");

    JSScope<20,20> scope(ctx);
    auto name = scope.toStdString(argv[0]);
    if (name.empty()) return JSI_EXCEPTION;

    cc->methods[name] = JSValueGuard(ctx, JSI_DupValue(ctx, argv[1]));
    return JSI_UNDEFINED;
}

// ---- registerView ----
JSValue jsi_registerView(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 2 || !JSI_IsString(argv[0]) || !JSI_IsFunction(ctx, argv[1]))
        return JSI_ThrowTypeError(ctx, "registerView(name, fn)");

    ContractContext* cc = (ContractContext*)JSI_GetContextOpaque(ctx);
    if (!cc) return JSI_ThrowTypeError(ctx, "no contract context");

    JSScope<20,20> scope(ctx);
    auto name = scope.toStdString(argv[0]);
    if (name.empty()) return JSI_EXCEPTION;

    cc->views[name] = JSValueGuard(ctx, JSI_DupValue(ctx, argv[1]));
    return JSI_UNDEFINED;
}

// ---- setGranule ----
JSValue jsi_setGranule(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 2 || !JSI_IsString(argv[0]))
        return JSI_ThrowTypeError(ctx, "setGranule(key, value)");

    ContractContext* cc = (ContractContext*)JSI_GetContextOpaque(ctx);
    if (!cc) return JSI_ThrowTypeError(ctx, "no contract context");

    JSScope<20,20> scope(ctx);
    auto key = scope.toStdString(argv[0]);
    if (key.empty()) return JSI_EXCEPTION;

    auto it = granules.find(key);
    if (it != granules.end() && it->second.owner_contract != cc->name)
        return JSI_ThrowTypeError(ctx, "access denied: granule owned by another contract");

    granules[key] = Granule{
        JSValueGuard(ctx, JSI_DupValue(ctx, argv[1])),
        cc->name
    };
    return JSI_UNDEFINED;
}

// ---- getGranule ----
JSValue jsi_getGranule(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 1 || !JSI_IsString(argv[0]))
        return JSI_ThrowTypeError(ctx, "getGranule(key)");

    ContractContext* cc = (ContractContext*)JSI_GetContextOpaque(ctx);
    if (!cc) return JSI_ThrowTypeError(ctx, "no contract context");

    JSScope<20,20> scope(ctx);
    auto key = scope.toStdString(argv[0]);
    if (key.empty()) return JSI_EXCEPTION;

    auto it = granules.find(key);
    if (it == granules.end()) return JSI_UNDEFINED;

    if (it->second.owner_contract != cc->name)
        return JSI_ThrowTypeError(ctx, "access denied: granule owned by another contract");

    return JSI_DupValue(ctx, it->second.value.get());
}

// ---- callView ----
JSValue jsi_callView(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 3 || !JSI_IsString(argv[0]) || !JSI_IsString(argv[1]))
        return JSI_ThrowTypeError(ctx, "callView(contract, view, params)");

    JSScope<20,20> scope(ctx);
    auto contract_name = scope.toStdString(argv[0]);
    auto view_name = scope.toStdString(argv[1]);
    if (contract_name.empty() || view_name.empty()) return JSI_EXCEPTION;

    // Нельзя вызывать свой же контракт через callView — только напрямую
    ContractContext* self = (ContractContext*)JSI_GetContextOpaque(ctx);
    if (self && self->name == contract_name)
        return JSI_ThrowTypeError(ctx, "callView to self is not allowed; call directly");

    auto it = contract_cache.find(contract_name);
    if (it == contract_cache.end())
        return JSI_ThrowTypeError(ctx, "contract not found: %s", contract_name.c_str());

    auto cc = it->second;
    auto vit = cc->views.find(view_name);
    if (vit == cc->views.end())
        return JSI_ThrowTypeError(ctx, "view not found: %s.%s",
            contract_name.c_str(), view_name.c_str());

    JSValue params_in_cc = convert_value(ctx, cc->ctx, argv[2]);
    JSValue result = JSI_Call(cc->ctx, vit->second.get(), JSI_UNDEFINED, 1, &params_in_cc);
    JSI_FreeValue(cc->ctx, params_in_cc);

    if (JSI_IsException(result)) {
        // Печатаем исключение в контексте контракта
        JSValue exc = JSI_GetException(cc->ctx);
        const char* s = JSI_ToCString(cc->ctx, exc);
        if (s) { std::cerr << "callView " << contract_name << "." << view_name
                           << " exception: " << s << std::endl; JSI_FreeCString(cc->ctx, s); }
        JSI_FreeValue(cc->ctx, exc);
        JSI_FreeValue(cc->ctx, result);
        return JSI_EXCEPTION;
    }

    JSValue result_in_caller = convert_value(cc->ctx, ctx, result);
    JSI_FreeValue(cc->ctx, result);
    return result_in_caller;
}

// ---- register_api ----
void register_api(JSContext* ctx) {
    JSValue global = JSI_GetGlobalObject(ctx);

    JSI_SetPropertyStr(ctx, global, "registerMethod",
        JSI_NewCFunction(ctx, jsi_registerMethod, "registerMethod", 2));
    JSI_SetPropertyStr(ctx, global, "registerView",
        JSI_NewCFunction(ctx, jsi_registerView, "registerView", 2));
    JSI_SetPropertyStr(ctx, global, "setGranule",
        JSI_NewCFunction(ctx, jsi_setGranule, "setGranule", 2));
    JSI_SetPropertyStr(ctx, global, "getGranule",
        JSI_NewCFunction(ctx, jsi_getGranule, "getGranule", 1));
    JSI_SetPropertyStr(ctx, global, "callView",
        JSI_NewCFunction(ctx, jsi_callView, "callView", 3));

    JSI_FreeValue(ctx, global);
}
