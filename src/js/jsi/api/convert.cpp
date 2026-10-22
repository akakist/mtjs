#include "convert.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdio>
#include "contract_constants.h"

using json = nlohmann::json;

void print_exception(JSIContext* ctx) 
{
    JSIValue exc = JSI_GetException(ctx);
    const char* s = JSI_ToCString(ctx, exc);
    if (s) { std::cerr << "Exception: " << s << std::endl; JSI_FreeCString(ctx, s); }
    JSI_FreeValue(ctx, exc);
}

std::string serialize_value(JSIContext* ctx, JSIValueConst val, int depth) 
{
    if (depth > MAX_SERIALIZE_DEPTH) return "<too deep>";
    if (JSI_IsUndefined(val)) return "undefined";
    if (JSI_IsNull(val)) return "null";
    if (JSI_IsBool(val)) return JSI_ToBool(ctx, val) ? "true" : "false";

    if (JSI_VALUE_GET_TAG(val) == JSI_TAG_INT) {
        int64_t d = JSI_VALUE_GET_INT(val);
        char buf[32];
        std::snprintf(buf, sizeof buf, "%lld", (long long)d);
        return buf;
    }

    if (JSI_IsString(val)) {
        size_t len = 0;
        const char* s = JSI_ToCStringLen(ctx, &len, val);
        if (!s) return "null";
        std::string r = "\"";
        for (size_t i = 0; i < len; ++i) {
            unsigned char c = (unsigned char)s[i];
            switch (c) {
                case '"':  r += "\\\""; break;
                case '\\': r += "\\\\"; break;
                case '\n': r += "\\n";  break;
                case '\r': r += "\\r";  break;
                case '\t': r += "\\t";  break;
                default:
                    if (c < 0x20) {
                        char esc[8];
                        std::snprintf(esc, sizeof esc, "\\u%04x", c);
                        r += esc;
                    } else {
                        r += (char)c;
                    }
            }
        }
        r += '"';
        JSI_FreeCString(ctx, s);
        return r;
    }

    if (JSI_IsArray(ctx, val)) {
        std::string r = "[";
        JSIValue lv = JSI_GetPropertyStr(ctx, val, "length");
        int64_t len = 0; JSI_ToInt64(ctx, &len, lv); JSI_FreeValue(ctx, lv);
        for (int64_t i = 0; i < len; ++i) {
            if (i) r += ",";
            JSIValue item = JSI_GetPropertyUint32(ctx, val, (uint32_t)i);
            r += serialize_value(ctx, item, depth + 1);
            JSI_FreeValue(ctx, item);
        }
        return r + "]";
    }

    if (JSI_IsObject(val)) {
        JSIPropertyEnum* props = nullptr;
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
            JSIValue item = JSI_GetPropertyStr(ctx, val, k.c_str());
            r += serialize_value(ctx, item, depth + 1);
            JSI_FreeValue(ctx, item);
        }
        return r + "}";
    }

    return "<unknown>";
}

bool is_serializable(const std::string& s) {
    return s.find('<') == std::string::npos;
}

json json_from_jsi(JSIContext* ctx, JSIValueConst val) 
{
    if (JSI_IsNull(val)) return nullptr;
    if (JSI_IsUndefined(val)) throw std::runtime_error("undefined in .tx.js");
    if (JSI_IsBool(val)) return JSI_ToBool(ctx, val) != 0;
    if (JSI_VALUE_GET_TAG(val) == JSI_TAG_INT) {
        return JSI_VALUE_GET_INT(val);
    }
    if (JSI_IsString(val)) {
        size_t len = 0;
        const char* s = JSI_ToCStringLen(ctx, &len, val);
        if (!s) throw std::runtime_error("bad string");
        std::string r(s, len);
        JSI_FreeCString(ctx, s);
        return r;
    }
    if (JSI_IsArray(ctx, val)) {
        json arr = json::array();
        JSIValue lv = JSI_GetPropertyStr(ctx, val, "length");
        int64_t len = 0; JSI_ToInt64(ctx, &len, lv); JSI_FreeValue(ctx, lv);
        for (int64_t i = 0; i < len; ++i) {
            JSIValue item = JSI_GetPropertyUint32(ctx, val, (uint32_t)i);
            arr.push_back(json_from_jsi(ctx, item));
            JSI_FreeValue(ctx, item);
        }
        return arr;
    }
    if (JSI_IsObject(val)) {
        json obj = json::object();
        JSIPropertyEnum* props = nullptr;
        uint32_t count = 0;
        if (JSI_GetOwnPropertyNames(ctx, &props, &count, val,
                JSI_GPN_STRING_MASK | JSI_GPN_ENUM_ONLY) == 0) {
            for (uint32_t i = 0; i < count; ++i) {
                const char* k = JSI_AtomToCString(ctx, props[i].atom);
                if (k) {
                    JSIValue item = JSI_GetProperty(ctx, val, props[i].atom);
                    obj[std::string(k)] = json_from_jsi(ctx, item);
                    JSI_FreeValue(ctx, item);
                    JSI_FreeCString(ctx, k);
                }
                JSI_FreeAtom(ctx, props[i].atom);
            }
            jsi_free(ctx, props);
        }
        return obj;
    }
    throw std::runtime_error("unsupported JS value in .tx.js");
}

JSIValue json_to_jsi(JSIContext* ctx, const json& j) 
{
    if (j.is_null()) return JSI_NULL;
    if (j.is_boolean()) return JSI_NewBool(ctx, j.get<bool>());
    if (j.is_number_integer()) return JSI_NewInt64(ctx, j.get<int64_t>());
    if (j.is_number_unsigned()) {
        uint64_t u = j.get<uint64_t>();
        if (u > (uint64_t)INT64_MAX)
            return JSI_ThrowTypeError(ctx, "uint64 too large");
        return JSI_NewInt64(ctx, (int64_t)u);
    }
    if (j.is_number_float())
        return JSI_ThrowTypeError(ctx, "float not supported");
    if (j.is_string()) {
        auto s = j.get<std::string>();
        return JSI_NewStringLen(ctx, s.data(), s.size());
    }
    if (j.is_array()) {
        JSIValue arr = JSI_NewArray(ctx);
        if (JSI_IsException(arr)) return arr;
        for (size_t i = 0; i < j.size(); ++i) {
            JSIValue v = json_to_jsi(ctx, j[i]);
            if (JSI_IsException(v)) { JSI_FreeValue(ctx, arr); return v; }
            JSI_SetPropertyUint32(ctx, arr, (uint32_t)i, v);
        }
        return arr;
    }
    if (j.is_object()) {
        JSIValue obj = JSI_NewObject(ctx);
        if (JSI_IsException(obj)) return obj;
        for (auto it = j.begin(); it != j.end(); ++it) {
            JSIValue v = json_to_jsi(ctx, it.value());
            if (JSI_IsException(v)) { JSI_FreeValue(ctx, obj); return v; }
            JSI_SetPropertyStr(ctx, obj, it.key().c_str(), v);
        }
        return obj;
    }
    return JSI_ThrowTypeError(ctx, "unsupported json type");
}

JSIValue convert_value(JSIContext* from_ctx, JSIContext* to_ctx, JSIValueConst val) {
    std::string s = serialize_value(from_ctx, val);
    if (!is_serializable(s)) {
        return JSI_ThrowTypeError(to_ctx, "cannot convert value: %s", s.c_str());
    }
    std::string wrapped = "(" + s + ")";
    JSIValue result = JSI_Eval(to_ctx, wrapped.c_str(), wrapped.size(),
                             "<convert>", JSI_EVAL_TYPE_GLOBAL);
    if (JSI_IsException(result)) {
        std::cerr << "convert_value failed, source: [" << wrapped << "]" << std::endl;
        print_exception(to_ctx);
    }
    return result;
}
