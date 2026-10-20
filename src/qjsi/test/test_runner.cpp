#include <quickjs_i.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <set>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <sys/stat.h>

#include "jsValueGuard.h"
#include "jsscope.h"

// ============================================================
// ContractContext
// ============================================================
struct ContractContext {
    JSRuntime* rt = nullptr;
    JSIContext* ctx = nullptr;
    std::string name;
    std::string path;
    bool in_view = false;
    std::map<std::string, JSIValueGuard> methods;
    std::map<std::string, JSIValueGuard> views;

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

// ============================================================
// Granule
// ============================================================
struct Granule {
    std::string value;
    int64_t create_time = 0;
    int64_t ttl = 0;   // 0 = бесконечно
};

// ============================================================
// Глобальное состояние
// ============================================================
std::map<std::string, Granule> granules;
std::map<std::string, std::shared_ptr<ContractContext>> contract_cache;

struct Event {
    std::string contract;
    std::string name;
    std::string value;
};

std::vector<Event> current_events;    // события текущей команды/транзакции
std::vector<Event> all_events;        // события всего прогона

static int64_t g_current_time = 0;
static int64_t g_current_block_height = 0;

static constexpr size_t MAX_GRANULE_SIZE = 10 * 1024;

// ============================================================
// Утилиты
// ============================================================
void print_exception(JSIContext* ctx) {
    JSIValue exc = JSI_GetException(ctx);
    const char* s = JSI_ToCString(ctx, exc);
    if (s) { std::cerr << "Exception: " << s << std::endl; JSI_FreeCString(ctx, s); }
    JSI_FreeValue(ctx, exc);
}

void delete_prop_str(JSIContext* ctx, JSIValueConst obj, const char* name) {
    JSAtom atom = JSI_NewAtom(ctx, name);
    if (atom != JSI_ATOM_NULL) {
        JSI_DeleteProperty(ctx, obj, atom, 0);
        JSI_FreeAtom(ctx, atom);
    }
}

std::string serialize_value(JSIContext* ctx, JSIValueConst val) {
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
            JSIValue item = JSI_GetPropertyStr(ctx, val, k.c_str());
            r += serialize_value(ctx, item);
            JSI_FreeValue(ctx, item);
        }
        return r + "}";
    }

    return "<unknown>";
}

bool is_serializable(const std::string& s) {
    return s.find('<') == std::string::npos;
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

// ============================================================
// API контракта
// ============================================================
JSIValue jsi_registerMethod(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    if (argc < 2 || !JSI_IsString(argv[0]) || !JSI_IsFunction(ctx, argv[1]))
        return JSI_ThrowTypeError(ctx, "registerMethod(name, fn)");
    ContractContext* cc = (ContractContext*)JSI_GetContextOpaque(ctx);
    if (!cc) return JSI_ThrowTypeError(ctx, "no contract context");
    JSScope<20,20> scope(ctx);
    auto name = scope.toStdString(argv[0]);
    if (name.empty()) return JSI_EXCEPTION;
    cc->methods[name] = JSIValueGuard(ctx, JSI_DupValue(ctx, argv[1]));
    return JSI_UNDEFINED;
}

JSIValue jsi_registerView(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    if (argc < 2 || !JSI_IsString(argv[0]) || !JSI_IsFunction(ctx, argv[1]))
        return JSI_ThrowTypeError(ctx, "registerView(name, fn)");
    ContractContext* cc = (ContractContext*)JSI_GetContextOpaque(ctx);
    if (!cc) return JSI_ThrowTypeError(ctx, "no contract context");
    JSScope<20,20> scope(ctx);
    auto name = scope.toStdString(argv[0]);
    if (name.empty()) return JSI_EXCEPTION;
    cc->views[name] = JSIValueGuard(ctx, JSI_DupValue(ctx, argv[1]));
    return JSI_UNDEFINED;
}

// createGranule(key, value, ttl) — создаёт, если уже есть -> TypeError
JSIValue jsi_createGranule(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    if (argc < 2 || !JSI_IsString(argv[0]))
        return JSI_ThrowTypeError(ctx, "createGranule(key, value[, ttl])");
    if (JSI_IsUndefined(argv[1]))
        return JSI_ThrowTypeError(ctx, "createGranule: undefined not allowed");
    ContractContext* cc = (ContractContext*)JSI_GetContextOpaque(ctx);
    if (!cc) return JSI_ThrowTypeError(ctx, "no contract context");
    if (cc->in_view)
        return JSI_ThrowTypeError(ctx, "createGranule is not allowed in view");

    JSScope<20,20> scope(ctx);
    auto key = scope.toStdString(argv[0]);
    if (key.empty()) return JSI_EXCEPTION;

    int64_t ttl = 0;
    if (argc >= 3 && !JSI_IsUndefined(argv[2])) {
        if (JSI_ToInt64(ctx, &ttl, argv[2]) != 0)
            return JSI_ThrowTypeError(ctx, "createGranule: ttl must be int64");
        if (ttl < 0)
            return JSI_ThrowTypeError(ctx, "createGranule: ttl must be >= 0");
    }

    std::string full_key = cc->name + "/" + key;
    if (granules.count(full_key))
        return JSI_ThrowTypeError(ctx, "createGranule: already exists: %s", full_key.c_str());

    std::string s = serialize_value(ctx, argv[1]);
    if (!is_serializable(s))
        return JSI_ThrowTypeError(ctx, "createGranule(%s): cannot serialize value", full_key.c_str());
    if (s.size() > MAX_GRANULE_SIZE)
        return JSI_ThrowTypeError(ctx, "createGranule(%s): value too large (%zu > %zu)",
                                 full_key.c_str(), s.size(), MAX_GRANULE_SIZE);

    granules[full_key] = Granule{ std::move(s), g_current_time, ttl };
    return JSI_UNDEFINED;
}

// setGranule(key, value) — обновляет существующую, иначе TypeError
JSIValue jsi_setGranule(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    if (argc < 2 || !JSI_IsString(argv[0]))
        return JSI_ThrowTypeError(ctx, "setGranule(key, value)");
    if (JSI_IsUndefined(argv[1]))
        return JSI_ThrowTypeError(ctx, "setGranule: undefined not allowed");
    ContractContext* cc = (ContractContext*)JSI_GetContextOpaque(ctx);
    if (!cc) return JSI_ThrowTypeError(ctx, "no contract context");
    if (cc->in_view)
        return JSI_ThrowTypeError(ctx, "setGranule is not allowed in view");

    JSScope<20,20> scope(ctx);
    auto key = scope.toStdString(argv[0]);
    if (key.empty()) return JSI_EXCEPTION;

    std::string full_key = cc->name + "/" + key;
    auto it = granules.find(full_key);
    if (it == granules.end())
        return JSI_ThrowTypeError(ctx, "setGranule: not found: %s", full_key.c_str());

    std::string s = serialize_value(ctx, argv[1]);
    if (!is_serializable(s))
        return JSI_ThrowTypeError(ctx, "setGranule(%s): cannot serialize value", full_key.c_str());
    if (s.size() > MAX_GRANULE_SIZE)
        return JSI_ThrowTypeError(ctx, "setGranule(%s): value too large (%zu > %zu)",
                                 full_key.c_str(), s.size(), MAX_GRANULE_SIZE);

    it->second.value = std::move(s);
    // create_time и ttl не меняем
    return JSI_UNDEFINED;
}

JSIValue jsi_getGranule(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    if (argc < 1 || !JSI_IsString(argv[0]))
        return JSI_ThrowTypeError(ctx, "getGranule(key)");
    ContractContext* cc = (ContractContext*)JSI_GetContextOpaque(ctx);
    if (!cc) return JSI_ThrowTypeError(ctx, "no contract context");
    JSScope<20,20> scope(ctx);
    auto key = scope.toStdString(argv[0]);
    if (key.empty()) return JSI_EXCEPTION;
    std::string full_key = cc->name + "/" + key;
    auto it = granules.find(full_key);
    if (it == granules.end()) return JSI_UNDEFINED;
    std::string wrapped = "(" + it->second.value + ")";
    return JSI_Eval(ctx, wrapped.c_str(), wrapped.size(), "<granule>", JSI_EVAL_TYPE_GLOBAL);
}

// emit(name, value) — запрещён во view, только в методе
JSIValue jsi_emit(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    if (argc < 2 || !JSI_IsString(argv[0]))
        return JSI_ThrowTypeError(ctx, "emit(name, value)");
    ContractContext* cc = (ContractContext*)JSI_GetContextOpaque(ctx);
    if (!cc) return JSI_ThrowTypeError(ctx, "no contract context");
    if (cc->in_view)
        return JSI_ThrowTypeError(ctx, "emit is not allowed in view");
    JSScope<20,20> scope(ctx);
    auto name = scope.toStdString(argv[0]);
    if (name.empty()) return JSI_EXCEPTION;
    std::string s = serialize_value(ctx, argv[1]);
    if (!is_serializable(s))
        return JSI_ThrowTypeError(ctx, "emit(%s): cannot serialize value", name.c_str());
    current_events.push_back(Event{ cc->name, std::move(name), std::move(s) });
    return JSI_UNDEFINED;
}

// ============================================================
// Вспомогательные: построение msg-объекта и вызов метода/вью
// ============================================================
struct Msg {
    std::string sender;
    int64_t value = 0;
    int64_t gasLimit = 0;
    int64_t gasUsed = 0;
    int64_t blockTimestamp = 0;
    int64_t blockHeight = 0;
    int64_t nonce = 0;
    int64_t chainId = 0;
    std::string txHash;
    int64_t txIndex = 0;
};

JSIValue build_msg_object(JSIContext* ctx, const Msg& m) {
    JSIValue obj = JSI_NewObject(ctx);
    JSI_SetPropertyStr(ctx, obj, "sender", JSI_NewString(ctx, m.sender.c_str()));
    JSI_SetPropertyStr(ctx, obj, "value", JSI_NewInt64(ctx, m.value));
    JSI_SetPropertyStr(ctx, obj, "gasLimit", JSI_NewInt64(ctx, m.gasLimit));
    JSI_SetPropertyStr(ctx, obj, "gasUsed", JSI_NewInt64(ctx, m.gasUsed));
    JSI_SetPropertyStr(ctx, obj, "blockTimestamp", JSI_NewInt64(ctx, m.blockTimestamp));
    JSI_SetPropertyStr(ctx, obj, "blockHeight", JSI_NewInt64(ctx, m.blockHeight));
    JSI_SetPropertyStr(ctx, obj, "nonce", JSI_NewInt64(ctx, m.nonce));
    JSI_SetPropertyStr(ctx, obj, "chainId", JSI_NewInt64(ctx, m.chainId));
    JSI_SetPropertyStr(ctx, obj, "txHash", JSI_NewString(ctx, m.txHash.c_str()));
    JSI_SetPropertyStr(ctx, obj, "txIndex", JSI_NewInt64(ctx, m.txIndex));
    return obj;
}

JSIValue call_contract_method(ContractContext* cc, JSIValueConst fn,
                             JSIValueConst params_js, const Msg& m, bool is_view)
{
    JSIValue msg_obj = build_msg_object(cc->ctx, m);
    JSIValue argv[2] = { params_js, msg_obj };
    int nargs = is_view ? 1 : 2;

    bool saved = cc->in_view;
    cc->in_view = is_view;
    JSIValue r = JSI_Call(cc->ctx, fn, JSI_UNDEFINED, nargs, argv);
    cc->in_view = saved;

    JSI_FreeValue(cc->ctx, msg_obj);
    return r;
}

// callMethod(contract, method, params) — из контракта
JSIValue jsi_callMethod(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    if (argc < 3 || !JSI_IsString(argv[0]) || !JSI_IsString(argv[1]))
        return JSI_ThrowTypeError(ctx, "callMethod(contract, method, params)");
    JSScope<20,20> scope(ctx);
    auto contract_name = scope.toStdString(argv[0]);
    auto method_name   = scope.toStdString(argv[1]);
    if (contract_name.empty() || method_name.empty()) return JSI_EXCEPTION;

    ContractContext* self = (ContractContext*)JSI_GetContextOpaque(ctx);
    if (self && self->name == contract_name)
        return JSI_ThrowTypeError(ctx, "callMethod to self not allowed");

    auto it = contract_cache.find(contract_name);
    if (it == contract_cache.end())
        return JSI_ThrowTypeError(ctx, "contract not found: %s", contract_name.c_str());
    auto cc = it->second;
    auto mit = cc->methods.find(method_name);
    if (mit == cc->methods.end())
        return JSI_ThrowTypeError(ctx, "method not found: %s.%s",
                                 contract_name.c_str(), method_name.c_str());

    JSIValue params_in_cc = convert_value(ctx, cc->ctx, argv[2]);
    if (JSI_IsException(params_in_cc)) return JSI_EXCEPTION;

    Msg empty;   // cross-contract: msg не пробрасываем (упрощение)
    JSIValue r = call_contract_method(cc.get(), mit->second.get(), params_in_cc, empty, false);
    JSI_FreeValue(cc->ctx, params_in_cc);

    if (JSI_IsException(r)) {
        print_exception(cc->ctx);
        JSI_FreeValue(cc->ctx, r);
        return JSI_EXCEPTION;
    }
    JSIValue r_caller = convert_value(cc->ctx, ctx, r);
    JSI_FreeValue(cc->ctx, r);
    return r_caller;
}

JSIValue jsi_callView(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    if (argc < 3 || !JSI_IsString(argv[0]) || !JSI_IsString(argv[1]))
        return JSI_ThrowTypeError(ctx, "callView(contract, view, params)");
    JSScope<20,20> scope(ctx);
    auto contract_name = scope.toStdString(argv[0]);
    auto view_name     = scope.toStdString(argv[1]);
    if (contract_name.empty() || view_name.empty()) return JSI_EXCEPTION;

    ContractContext* self = (ContractContext*)JSI_GetContextOpaque(ctx);
    if (self && self->name == contract_name)
        return JSI_ThrowTypeError(ctx, "callView to self not allowed");

    auto it = contract_cache.find(contract_name);
    if (it == contract_cache.end())
        return JSI_ThrowTypeError(ctx, "contract not found: %s", contract_name.c_str());
    auto cc = it->second;
    auto vit = cc->views.find(view_name);
    if (vit == cc->views.end())
        return JSI_ThrowTypeError(ctx, "view not found: %s.%s",
                                 contract_name.c_str(), view_name.c_str());

    JSIValue params_in_cc = convert_value(ctx, cc->ctx, argv[2]);
    if (JSI_IsException(params_in_cc)) return JSI_EXCEPTION;

    Msg empty;
    JSIValue r = call_contract_method(cc.get(), vit->second.get(), params_in_cc, empty, true);
    JSI_FreeValue(cc->ctx, params_in_cc);

    if (JSI_IsException(r)) {
        print_exception(cc->ctx);
        JSI_FreeValue(cc->ctx, r);
        return JSI_EXCEPTION;
    }
    JSIValue r_caller = convert_value(cc->ctx, ctx, r);
    JSI_FreeValue(cc->ctx, r);
    return r_caller;
}

void register_api(JSIContext* ctx) {
    JSIValue global = JSI_GetGlobalObject(ctx);
    JSI_SetPropertyStr(ctx, global, "registerMethod",
        JSI_NewCFunction(ctx, jsi_registerMethod, "registerMethod", 2));
    JSI_SetPropertyStr(ctx, global, "registerView",
        JSI_NewCFunction(ctx, jsi_registerView, "registerView", 2));
    JSI_SetPropertyStr(ctx, global, "createGranule",
        JSI_NewCFunction(ctx, jsi_createGranule, "createGranule", 3));
    JSI_SetPropertyStr(ctx, global, "setGranule",
        JSI_NewCFunction(ctx, jsi_setGranule, "setGranule", 2));
    JSI_SetPropertyStr(ctx, global, "getGranule",
        JSI_NewCFunction(ctx, jsi_getGranule, "getGranule", 1));
    JSI_SetPropertyStr(ctx, global, "emit",
        JSI_NewCFunction(ctx, jsi_emit, "emit", 2));
    JSI_SetPropertyStr(ctx, global, "callMethod",
        JSI_NewCFunction(ctx, jsi_callMethod, "callMethod", 3));
    JSI_SetPropertyStr(ctx, global, "callView",
        JSI_NewCFunction(ctx, jsi_callView, "callView", 3));
    JSI_FreeValue(ctx, global);
}

// ============================================================
// Загрузка контракта
// ============================================================
std::shared_ptr<ContractContext> load_contract(
    const std::string& name, const std::string& path)
{
    auto it = contract_cache.find(name);
    if (it != contract_cache.end()) return it->second;

    auto cc = std::make_shared<ContractContext>();
    cc->name = name;
    cc->path = path;

    if (!cc->create()) { std::cerr << "cannot create context for " << name << std::endl; return nullptr; }

    register_api(cc->ctx);

    JSIValue global = JSI_GetGlobalObject(cc->ctx);
    const char* banned[] = {
        "Math","Date","RegExp","JSON","Proxy","Promise","Map","Set",
        "WeakRef","FinalizationRegistry","Atomics","SharedArrayBuffer"
    };
    for (auto b : banned) delete_prop_str(cc->ctx, global, b);
    JSI_FreeValue(cc->ctx, global);

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

    JSIValue compiled = JSI_Eval(cc->ctx, src.data(), src.size(), path.c_str(),
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

std::shared_ptr<ContractContext> get_contract(
    const std::string& name, const std::string& contracts_dir)
{
    auto it = contract_cache.find(name);
    if (it != contract_cache.end()) return it->second;
    return load_contract(name, contracts_dir + "/" + name + ".js");
}

// ============================================================
// Тест
// ============================================================
struct TxCommand {
    std::string contract;
    std::string method;
    JSIValueGuard params;
    bool expect_error = false;
    JSIValueGuard expect_result;
    std::vector<Event> expect_events;
};

struct Transaction {
    Msg msg;
    std::vector<TxCommand> commands;
};

struct TestCase {
    int64_t time = 0;
    std::map<std::string, JSIValueGuard> initial;
    std::vector<Transaction> transactions;
    std::vector<Event> expected_emits;
    std::map<std::string, JSIValueGuard> expected;
};

static void read_object_map(JSIContext* ctx, JSIValueConst obj,
                            std::map<std::string, JSIValueGuard>& out)
{
    JSPropertyEnum* props = nullptr;
    uint32_t count = 0;
    if (JSI_GetOwnPropertyNames(ctx, &props, &count, obj,
            JSI_GPN_STRING_MASK | JSI_GPN_ENUM_ONLY) != 0) return;
    for (uint32_t i = 0; i < count; ++i) {
        const char* k = JSI_AtomToCString(ctx, props[i].atom);
        if (k) {
            JSIValue v = JSI_GetProperty(ctx, obj, props[i].atom);
            out[k] = JSIValueGuard(ctx, JSI_DupValue(ctx, v));
            JSI_FreeValue(ctx, v);
            JSI_FreeCString(ctx, k);
        }
        JSI_FreeAtom(ctx, props[i].atom);
    }
    jsi_free(ctx, props);
}

static std::string read_string_prop(JSIContext* ctx, JSIValueConst obj, const char* name) {
    JSIValue v = JSI_GetPropertyStr(ctx, obj, name);
    std::string r;
    if (JSI_IsString(v)) { const char* s = JSI_ToCString(ctx, v);
        if (s) { r = s; JSI_FreeCString(ctx, s); } }
    JSI_FreeValue(ctx, v);
    return r;
}

static int64_t read_int64_prop(JSIContext* ctx, JSIValueConst obj, const char* name) {
    JSIValue v = JSI_GetPropertyStr(ctx, obj, name);
    int64_t r = 0;
    if (!JSI_IsUndefined(v)) JSI_ToInt64(ctx, &r, v);
    JSI_FreeValue(ctx, v);
    return r;
}

static Msg read_msg(JSIContext* ctx, JSIValueConst obj) {
    Msg m;
    if (!JSI_IsObject(obj)) return m;
    m.sender         = read_string_prop(ctx, obj, "sender");
    m.value          = read_int64_prop(ctx, obj, "value");
    m.gasLimit       = read_int64_prop(ctx, obj, "gasLimit");
    m.blockTimestamp = read_int64_prop(ctx, obj, "blockTimestamp");
    m.blockHeight    = read_int64_prop(ctx, obj, "blockHeight");
    m.nonce          = read_int64_prop(ctx, obj, "nonce");
    m.chainId        = read_int64_prop(ctx, obj, "chainId");
    m.txHash         = read_string_prop(ctx, obj, "txHash");
    m.txIndex        = read_int64_prop(ctx, obj, "txIndex");
    return m;
}

static void read_events(JSIContext* ctx, JSIValueConst arr, std::vector<Event>& out) {
    if (!JSI_IsArray(ctx, arr)) return;
    JSIValue lv = JSI_GetPropertyStr(ctx, arr, "length");
    int64_t len = 0; JSI_ToInt64(ctx, &len, lv); JSI_FreeValue(ctx, lv);
    for (int64_t i = 0; i < len; ++i) {
        JSIValue e = JSI_GetPropertyUint32(ctx, arr, (uint32_t)i);
        if (!JSI_IsObject(e)) { JSI_FreeValue(ctx, e); continue; }
        Event ev;
        ev.contract = read_string_prop(ctx, e, "contract");
        ev.name     = read_string_prop(ctx, e, "name");
        JSIValue vv  = JSI_GetPropertyStr(ctx, e, "value");
        ev.value    = serialize_value(ctx, vv);
        JSI_FreeValue(ctx, vv);
        out.push_back(std::move(ev));
        JSI_FreeValue(ctx, e);
    }
}

bool load_test(JSIContext* ctx, const std::string& path, TestCase& out) {
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

    JSIValue compiled = JSI_Eval(ctx, src.data(), src.size(), path.c_str(),
        JSI_EVAL_FLAG_STRICT | JSI_EVAL_TYPE_GLOBAL | JSI_EVAL_FLAG_COMPILE_ONLY);
    if (JSI_IsException(compiled)) { print_exception(ctx); return false; }

    JSIValue result = JSI_EvalFunction(ctx, compiled);
    if (JSI_IsException(result)) { print_exception(ctx); JSI_FreeValue(ctx, result); return false; }
    if (!JSI_IsObject(result)) {
        std::cerr << path << ": result is not an object" << std::endl;
        JSI_FreeValue(ctx, result); return false;
    }

    out.time = read_int64_prop(ctx, result, "time");

    JSIValue initial_val = JSI_GetPropertyStr(ctx, result, "initial");
    if (JSI_IsObject(initial_val)) read_object_map(ctx, initial_val, out.initial);
    JSI_FreeValue(ctx, initial_val);

    JSIValue txs_val = JSI_GetPropertyStr(ctx, result, "transactions");
    if (JSI_IsArray(ctx, txs_val)) {
        JSIValue lv = JSI_GetPropertyStr(ctx, txs_val, "length");
        int64_t len = 0; JSI_ToInt64(ctx, &len, lv); JSI_FreeValue(ctx, lv);

        for (int64_t i = 0; i < len; ++i) {
            JSIValue item = JSI_GetPropertyUint32(ctx, txs_val, (uint32_t)i);
            if (!JSI_IsObject(item)) { JSI_FreeValue(ctx, item); continue; }

            Transaction tx;
            JSIValue mv = JSI_GetPropertyStr(ctx, item, "msg");
            tx.msg = read_msg(ctx, mv);
            JSI_FreeValue(ctx, mv);

            JSIValue cmds = JSI_GetPropertyStr(ctx, item, "commands");
            if (JSI_IsArray(ctx, cmds)) {
                JSIValue clv = JSI_GetPropertyStr(ctx, cmds, "length");
                int64_t clen = 0; JSI_ToInt64(ctx, &clen, clv); JSI_FreeValue(ctx, clv);
                for (int64_t j = 0; j < clen; ++j) {
                    JSIValue c = JSI_GetPropertyUint32(ctx, cmds, (uint32_t)j);
                    if (!JSI_IsObject(c)) { JSI_FreeValue(ctx, c); continue; }
                    TxCommand cmd;
                    cmd.contract = read_string_prop(ctx, c, "contract");
                    cmd.method   = read_string_prop(ctx, c, "method");
                    JSIValue pv = JSI_GetPropertyStr(ctx, c, "params");
                    cmd.params = JSIValueGuard(ctx,
                        JSI_IsUndefined(pv) ? JSI_NewObject(ctx) : JSI_DupValue(ctx, pv));
                    JSI_FreeValue(ctx, pv);
                    JSIValue ev = JSI_GetPropertyStr(ctx, c, "expect_error");
                    if (!JSI_IsUndefined(ev)) cmd.expect_error = JSI_ToBool(ctx, ev);
                    JSI_FreeValue(ctx, ev);
                    JSIValue rv = JSI_GetPropertyStr(ctx, c, "expect_result");
                    if (!JSI_IsUndefined(rv)) cmd.expect_result = JSIValueGuard(ctx, JSI_DupValue(ctx, rv));
                    JSI_FreeValue(ctx, rv);
                    JSIValue ee = JSI_GetPropertyStr(ctx, c, "expect_events");
                    read_events(ctx, ee, cmd.expect_events);
                    JSI_FreeValue(ctx, ee);
                    tx.commands.push_back(std::move(cmd));
                    JSI_FreeValue(ctx, c);
                }
            }
            JSI_FreeValue(ctx, cmds);
            out.transactions.push_back(std::move(tx));
            JSI_FreeValue(ctx, item);
        }
    }
    JSI_FreeValue(ctx, txs_val);

    JSIValue ee_val = JSI_GetPropertyStr(ctx, result, "expected_emits");
    read_events(ctx, ee_val, out.expected_emits);
    JSI_FreeValue(ctx, ee_val);

    JSIValue expected_val = JSI_GetPropertyStr(ctx, result, "expected");
    if (JSI_IsObject(expected_val)) read_object_map(ctx, expected_val, out.expected);
    JSI_FreeValue(ctx, expected_val);

    JSI_FreeValue(ctx, result);
    return true;
}

// ============================================================
// Прогон
// ============================================================
static void apply_initial(JSIContext* ctx, const TestCase& tc) {
    granules.clear();
    all_events.clear();
    current_events.clear();
    g_current_time = tc.time;
    for (auto& [key, guard] : tc.initial) {
        std::string s = serialize_value(ctx, guard.get());
        if (!is_serializable(s)) {
            std::cerr << "initial[" << key << "]: cannot serialize " << s << std::endl;
            continue;
        }
        granules[key] = Granule{ std::move(s), 0, 0 };
    }
}
static bool run_transactions(JSIContext* runner_ctx, TestCase& tc) {
    for (auto& tx : tc.transactions) {
        auto saved = granules;
        auto saved_events = all_events;
        bool tx_aborted = false;

        for (auto& cmd : tx.commands) {
            auto cc = get_contract(cmd.contract, "contracts");
            if (!cc) { std::cerr << "contract not loaded: " << cmd.contract << std::endl; return false; }
            auto mit = cc->methods.find(cmd.method);
            if (mit == cc->methods.end()) {
                std::cerr << "method not found: " << cmd.contract << "." << cmd.method << std::endl;
                return false;
            }

            JSIValue params_in_cc = convert_value(runner_ctx, cc->ctx, cmd.params.get());
            if (JSI_IsException(params_in_cc)) {
                print_exception(cc->ctx);
                granules = std::move(saved);
                all_events = std::move(saved_events);
                return false;
            }

            current_events.clear();
            JSIValue result = call_contract_method(cc.get(), mit->second.get(), params_in_cc, tx.msg, false);
            JSI_FreeValue(cc->ctx, params_in_cc);

            if (JSI_IsException(result)) {
                JSI_FreeValue(cc->ctx, result);
                granules = std::move(saved);
                all_events = std::move(saved_events);
                current_events.clear();
                if (!cmd.expect_error) {
                    std::cerr << "unexpected exception in " << cmd.contract << "." << cmd.method << std::endl;
                    return false;
                }
                tx_aborted = true;
                break;
            }
            if (cmd.expect_error) {
                std::cerr << "expected error, but got result in "
                          << cmd.contract << "." << cmd.method << std::endl;
                JSI_FreeValue(cc->ctx, result);
                granules = std::move(saved);
                all_events = std::move(saved_events);
                return false;
            }

            if (cmd.expect_result) {
                std::string exp = serialize_value(runner_ctx, cmd.expect_result.get());
                JSIValue res_in_runner = convert_value(cc->ctx, runner_ctx, result);
                if (JSI_IsException(res_in_runner)) {
                    print_exception(runner_ctx);
                    JSI_FreeValue(cc->ctx, result);
                    granules = std::move(saved);
                    all_events = std::move(saved_events);
                    return false;
                }
                std::string act = serialize_value(runner_ctx, res_in_runner);
                JSI_FreeValue(runner_ctx, res_in_runner);
                if (exp != act) {
                    std::cerr << "result mismatch in " << cmd.contract << "." << cmd.method
                              << ": expected " << exp << ", got " << act << std::endl;
                    JSI_FreeValue(cc->ctx, result);
                    granules = std::move(saved);
                    all_events = std::move(saved_events);
                    return false;
                }
            }

            // if (!cmd.expect_events.empty() || !current_events.empty()) 
            if (!cmd.expect_events.empty()) {
                if (current_events.size() != cmd.expect_events.size()) {
                    std::cerr << "  events count mismatch in " << cmd.contract << "." << cmd.method
                              << ": expected " << cmd.expect_events.size()
                              << ", got " << current_events.size() << std::endl;
                    JSI_FreeValue(cc->ctx, result);
                    granules = std::move(saved);
                    all_events = std::move(saved_events);
                    return false;
                }
                for (size_t k = 0; k < cmd.expect_events.size(); ++k) {
                    if (cmd.expect_events[k].contract != current_events[k].contract ||
                        cmd.expect_events[k].name     != current_events[k].name ||
                        cmd.expect_events[k].value    != current_events[k].value) {
                        std::cerr << "  event[" << k << "] mismatch in "
                                  << cmd.contract << "." << cmd.method << std::endl;
                        JSI_FreeValue(cc->ctx, result);
                        granules = std::move(saved);
                        all_events = std::move(saved_events);
                        return false;
                    }
                }
            }

            for (auto& ev : current_events) all_events.push_back(ev);
            JSI_FreeValue(cc->ctx, result);
        }

        if (tx_aborted) continue;
    }
    return true;
}
static bool compare_emits(const std::vector<Event>& actual,
                          const std::vector<Event>& expected)
{
    if (actual.size() != expected.size()) {
        std::cerr << "  emits count mismatch: expected " << expected.size()
                  << ", got " << actual.size() << std::endl;
        for (auto& e : expected)
            std::cerr << "    expected: " << e.contract << "." << e.name
                      << "(" << e.value << ")" << std::endl;
        for (auto& e : actual)
            std::cerr << "    actual:   " << e.contract << "." << e.name
                      << "(" << e.value << ")" << std::endl;
        return false;
    }
    for (size_t i = 0; i < actual.size(); ++i) {
        if (actual[i].contract != expected[i].contract ||
            actual[i].name     != expected[i].name ||
            actual[i].value    != expected[i].value) {
            std::cerr << "  emits[" << i << "] mismatch: expected "
                      << expected[i].contract << "." << expected[i].name
                      << "(" << expected[i].value << "), got "
                      << actual[i].contract << "." << actual[i].name
                      << "(" << actual[i].value << ")" << std::endl;
            return false;
        }
    }
    return true;
}

static bool compare_expected(JSIContext* ctx, const TestCase& tc) {
    bool ok = true;
    for (auto& [key, guard] : tc.expected) {
        auto it = granules.find(key);
        if (it == granules.end()) {
            std::cerr << "  missing granule: " << key << std::endl;
            ok = false; continue;
        }
        std::string exp = serialize_value(ctx, guard.get());
        if (exp != it->second.value) {
            std::cerr << "  mismatch at " << key
                      << ": expected " << exp
                      << ", got "      << it->second.value << std::endl;
            ok = false;
        }
    }
    return ok;
}

// ============================================================
// main
// ============================================================
std::vector<std::string> list_directory(const std::string& path) {
    std::vector<std::string> result;
    DIR* dir = opendir(path.c_str());
    if (!dir) return result;
    struct dirent* e;
    while ((e = readdir(dir)) != nullptr) {
        if (!std::strcmp(e->d_name, ".") || !std::strcmp(e->d_name, "..")) continue;
        std::string full = path + "/" + e->d_name;
        struct stat st;
        if (stat(full.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) continue;
        result.emplace_back(e->d_name);
    }
    closedir(dir);
    std::sort(result.begin(), result.end());
    return result;
}

int main() {
    JSRuntime* rt = JSI_NewRuntime();
    if (!rt) return 1;
    JSIContext* ctx = JSI_NewContext(rt);
    if (!ctx) { JSI_FreeRuntime(rt); return 1; }
    // g_runner_ctx = ctx;

    auto dir = list_directory("tx");
    int failures = 0, total = 0;

    for (auto& file : dir) {
        if (file.size() < 6 || file.substr(file.size() - 6) != ".tx.js") continue;
        total++;
        std::string path = "tx/" + file;
        std::cout << "Running: " << path << std::endl;

        TestCase tc;
        if (!load_test(ctx, path, tc)) { failures++; continue; }

        bool load_ok = true;
        for (auto& tx : tc.transactions)
            for (auto& cmd : tx.commands)
                if (!get_contract(cmd.contract, "contracts")) { load_ok = false; break; }
        if (!load_ok) { std::cerr << "  cannot load contracts" << std::endl; failures++; continue; }

        apply_initial(ctx, tc);

        if (!run_transactions(ctx, tc)) { failures++; continue; }
        if (!compare_emits(all_events, tc.expected_emits)) { failures++; continue; }
        if (!compare_expected(ctx, tc)) { failures++; continue; }

        std::cout << "  PASS" << std::endl;
    }

    granules.clear();
    contract_cache.clear();
    JSI_FreeContext(ctx);
    JSI_FreeRuntime(rt);

    std::cout << total - failures << "/" << total << " passed" << std::endl;
    return failures ? 1 : 0;
}
