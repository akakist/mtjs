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

static JSContext* g_runner_ctx = nullptr;

// ============================================================
// ContractContext
// ============================================================
struct ContractContext {
    JSRuntime* rt = nullptr;
    JSContext* ctx = nullptr;
    std::string name;
    std::string path;
    bool in_view = false;
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

// ============================================================
// Granule — только значение. Владелец не хранится (анонимизация
// на боевом через blake2b(contract || "/" || key)).
// ============================================================
struct Granule {
    std::string value;
};

// ============================================================
// Глобальное состояние
// ============================================================
std::map<std::string, Granule> granules;   // полный ключ "contract/key"
std::map<std::string, std::shared_ptr<ContractContext>> contract_cache;

// ============================================================
// Утилиты
// ============================================================
void print_exception(JSContext* ctx) {
    JSValue exc = JS_GetException(ctx);
    const char* s = JS_ToCString(ctx, exc);
    if (s) { std::cerr << "Exception: " << s << std::endl; JS_FreeCString(ctx, s); }
    JS_FreeValue(ctx, exc);
}

void delete_prop_str(JSContext* ctx, JSValueConst obj, const char* name) {
    JSAtom atom = JS_NewAtom(ctx, name);
    if (atom != JS_ATOM_NULL) {
        JS_DeleteProperty(ctx, obj, atom, 0);
        JS_FreeAtom(ctx, atom);
    }
}

// Строгая сериализация JSValue в валидный JS-литерал.
std::string serialize_value(JSContext* ctx, JSValueConst val) {
    if (JS_IsUndefined(val)) return "undefined";
    if (JS_IsNull(val)) return "null";
    if (JS_IsBool(val)) return JS_ToBool(ctx, val) ? "true" : "false";

    if ( val.tag == JS_TAG_FLOAT64 )
    printf("if ( val.tag == JS_TAG_FLOAT64 ) %f\n",val.u.float64);
    if ( val.tag == JS_TAG_INT ) {
        int64_t d;
        if (JS_ToInt64(ctx, &d, val) != 0) return "0";
        // if (std::isnan(d)) return "NaN";
        // if (std::isinf(d)) return d > 0 ? "Infinity" : "-Infinity";
        int64_t n;
        // if (d == std::trunc(d) && std::fabs(d) < 1e15 && JS_ToInt64(ctx, &n, val) == 0)
        //     return std::to_string(n);
        char buf[32];
        std::snprintf(buf, sizeof buf, "%ld", d);
        return buf;
    }

    if (JS_IsString(val)) {
        size_t len = 0;
        const char* s = JS_ToCStringLen(ctx, &len, val);
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

bool is_serializable(const std::string& s) {
    return s.find('<') == std::string::npos;
}

JSValue convert_value(JSContext* from_ctx, JSContext* to_ctx, JSValueConst val) {
    std::string s = serialize_value(from_ctx, val);
    if (!is_serializable(s)) {
        return JS_ThrowTypeError(to_ctx, "cannot convert value: %s", s.c_str());
    }
    std::string wrapped = "(" + s + ")";
    JSValue result = JS_Eval(to_ctx, wrapped.c_str(), wrapped.size(),
                             "<convert>", JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(result)) {
        std::cerr << "convert_value failed, source: [" << wrapped << "]" << std::endl;
        print_exception(to_ctx);
    }
    return result;
}

// ============================================================
// API контракта
// ============================================================
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

JSValue js_setGranule(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 2 || !JS_IsString(argv[0]))
        return JS_ThrowTypeError(ctx, "setGranule(key, value)");
    if (JS_IsUndefined(argv[1]))
        return JS_ThrowTypeError(ctx, "setGranule: undefined not allowed");

    ContractContext* cc = (ContractContext*)JS_GetContextOpaque(ctx);
    if (!cc) return JS_ThrowTypeError(ctx, "no contract context");
    if (cc->in_view)
        return JS_ThrowTypeError(ctx, "setGranule is not allowed in view");

    JSScope<20,20> scope(ctx);
    auto key = scope.toStdString(argv[0]);
    if (key.empty()) return JS_EXCEPTION;

    std::string full_key = cc->name + "/" + key;

    std::string s = serialize_value(ctx, argv[1]);
    if (!is_serializable(s))
        return JS_ThrowTypeError(ctx, "setGranule(%s): cannot serialize value",
                                 full_key.c_str());

    granules[full_key] = Granule{ std::move(s) };
    return JS_UNDEFINED;
}

JSValue js_getGranule(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 1 || !JS_IsString(argv[0]))
        return JS_ThrowTypeError(ctx, "getGranule(key)");

    ContractContext* cc = (ContractContext*)JS_GetContextOpaque(ctx);
    if (!cc) return JS_ThrowTypeError(ctx, "no contract context");

    JSScope<20,20> scope(ctx);
    auto key = scope.toStdString(argv[0]);
    if (key.empty()) return JS_EXCEPTION;

    std::string full_key = cc->name + "/" + key;

    auto it = granules.find(full_key);
    if (it == granules.end()) return JS_UNDEFINED;

    std::string wrapped = "(" + it->second.value + ")";
    return JS_Eval(ctx, wrapped.c_str(), wrapped.size(),
                   "<granule>", JS_EVAL_TYPE_GLOBAL);
}

JSValue js_call(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 3 || !JS_IsString(argv[0]) || !JS_IsString(argv[1]))
        return JS_ThrowTypeError(ctx, "call(contract, method, params)");

    JSScope<20,20> scope(ctx);
    auto contract_name = scope.toStdString(argv[0]);
    auto method_name   = scope.toStdString(argv[1]);
    if (contract_name.empty() || method_name.empty()) return JS_EXCEPTION;

    ContractContext* self = (ContractContext*)JS_GetContextOpaque(ctx);
    if (self && self->name == contract_name)
        return JS_ThrowTypeError(ctx, "call to self is not allowed; call directly");

    auto it = contract_cache.find(contract_name);
    if (it == contract_cache.end())
        return JS_ThrowTypeError(ctx, "contract not found: %s", contract_name.c_str());

    auto cc = it->second;
    auto mit = cc->methods.find(method_name);
    if (mit == cc->methods.end())
        return JS_ThrowTypeError(ctx, "method not found: %s.%s",
            contract_name.c_str(), method_name.c_str());

    JSValue params_in_cc = convert_value(ctx, cc->ctx, argv[2]);
    if (JS_IsException(params_in_cc)) return JS_EXCEPTION;

    bool saved_in_view = cc->in_view;
    cc->in_view = false;
    JSValue result = JS_Call(cc->ctx, mit->second.get(), JS_UNDEFINED, 1, &params_in_cc);
    cc->in_view = saved_in_view;
    JS_FreeValue(cc->ctx, params_in_cc);

    if (JS_IsException(result)) {
        JSValue exc = JS_GetException(cc->ctx);
        const char* s = JS_ToCString(cc->ctx, exc);
        if (s) { std::cerr << "call " << contract_name << "." << method_name
                           << " exception: " << s << std::endl; JS_FreeCString(cc->ctx, s); }
        JS_FreeValue(cc->ctx, exc);
        JS_FreeValue(cc->ctx, result);
        return JS_EXCEPTION;
    }

    JSValue result_in_caller = convert_value(cc->ctx, ctx, result);
    JS_FreeValue(cc->ctx, result);
    return result_in_caller;
}

JSValue js_callView(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 3 || !JS_IsString(argv[0]) || !JS_IsString(argv[1]))
        return JS_ThrowTypeError(ctx, "callView(contract, view, params)");

    JSScope<20,20> scope(ctx);
    auto contract_name = scope.toStdString(argv[0]);
    auto view_name     = scope.toStdString(argv[1]);
    if (contract_name.empty() || view_name.empty()) return JS_EXCEPTION;

    ContractContext* self = (ContractContext*)JS_GetContextOpaque(ctx);
    if (self && self->name == contract_name)
        return JS_ThrowTypeError(ctx, "callView to self is not allowed");

    auto it = contract_cache.find(contract_name);
    if (it == contract_cache.end())
        return JS_ThrowTypeError(ctx, "contract not found: %s", contract_name.c_str());

    auto cc = it->second;
    auto vit = cc->views.find(view_name);
    if (vit == cc->views.end())
        return JS_ThrowTypeError(ctx, "view not found: %s.%s",
            contract_name.c_str(), view_name.c_str());

    JSValue params_in_cc = convert_value(ctx, cc->ctx, argv[2]);
    if (JS_IsException(params_in_cc)) return JS_EXCEPTION;

    bool saved_in_view = cc->in_view;
    cc->in_view = true;
    JSValue result = JS_Call(cc->ctx, vit->second.get(), JS_UNDEFINED, 1, &params_in_cc);
    cc->in_view = saved_in_view;
    JS_FreeValue(cc->ctx, params_in_cc);

    if (JS_IsException(result)) {
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
    JS_SetPropertyStr(ctx, global, "call",
        JS_NewCFunction(ctx, js_call, "call", 3));
    JS_SetPropertyStr(ctx, global, "callView",
        JS_NewCFunction(ctx, js_callView, "callView", 3));
    JS_FreeValue(ctx, global);
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

    if (!cc->create()) {
        std::cerr << "cannot create context for " << name << std::endl;
        return nullptr;
    }

    register_api(cc->ctx);

    JSValue global = JS_GetGlobalObject(cc->ctx);
    const char* banned[] = {
        "Math","Date","RegExp","JSON","Proxy","Promise","Map","Set",
        "WeakRef","FinalizationRegistry","Atomics","SharedArrayBuffer"
    };
    for (auto b : banned) delete_prop_str(cc->ctx, global, b);
    JS_FreeValue(cc->ctx, global);

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

    JSValue compiled = JS_Eval(cc->ctx, src.data(), src.size(), path.c_str(),
        JS_EVAL_FLAG_STRICT | JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_COMPILE_ONLY);
    if (JS_IsException(compiled)) { print_exception(cc->ctx); return nullptr; }

    JSValue result = JS_EvalFunction(cc->ctx, compiled);
    if (JS_IsException(result)) {
        print_exception(cc->ctx);
        JS_FreeValue(cc->ctx, result);
        return nullptr;
    }
    JS_FreeValue(cc->ctx, result);

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

    JSValue initial_val = JS_GetPropertyStr(ctx, result, "initial");
    if (JS_IsObject(initial_val)) read_object_map(ctx, initial_val, out.initial);
    JS_FreeValue(ctx, initial_val);

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

    JSValue expected_val = JS_GetPropertyStr(ctx, result, "expected");
    if (JS_IsObject(expected_val)) read_object_map(ctx, expected_val, out.expected);
    JS_FreeValue(ctx, expected_val);

    JS_FreeValue(ctx, result);
    return true;
}

// ============================================================
// Применение initial / выполнение / сравнение
// ============================================================
static void apply_initial(JSContext* ctx, const TestCase& tc) {
    granules.clear();
    for (auto& [key, guard] : tc.initial) {
        std::string s = serialize_value(ctx, guard.get());
        if (!is_serializable(s)) {
            std::cerr << "initial[" << key << "]: cannot serialize " << s << std::endl;
            continue;
        }
        granules[key] = Granule{ std::move(s) };
    }
}

static bool run_transactions(JSContext* runner_ctx, TestCase& tc) {
    for (auto& tx : tc.transactions) {
        auto cc = get_contract(tx.contract, "contracts");
        if (!cc) { std::cerr << "contract not loaded: " << tx.contract << std::endl; return false; }

        auto mit = cc->methods.find(tx.method);
        if (mit == cc->methods.end()) {
            std::cerr << "method not found: " << tx.contract << "." << tx.method << std::endl;
            return false;
        }

        JSValue params_in_cc = convert_value(runner_ctx, cc->ctx, tx.params.get());
        if (JS_IsException(params_in_cc)) {
            print_exception(cc->ctx);
            return false;
        }

        bool saved_in_view = cc->in_view;
        cc->in_view = false;
        JSValue result = JS_Call(cc->ctx, mit->second.get(), JS_UNDEFINED, 1, &params_in_cc);
        cc->in_view = saved_in_view;
        JS_FreeValue(cc->ctx, params_in_cc);

        if (JS_IsException(result)) {
            if (!tx.expect_error) {
                std::cerr << "unexpected exception in " << tx.contract << "." << tx.method << std::endl;
                print_exception(cc->ctx);
                JS_FreeValue(cc->ctx, result);
                return false;
            }
            JS_FreeValue(cc->ctx, result);
        } else {
            if (tx.expect_error) {
                std::cerr << "expected error, but got result in "
                          << tx.contract << "." << tx.method << std::endl;
                JS_FreeValue(cc->ctx, result);
                return false;
            }
            if (tx.expect_result) {
                std::string exp = serialize_value(runner_ctx, tx.expect_result.get());
                JSValue res_in_runner = convert_value(cc->ctx, runner_ctx, result);
                if (JS_IsException(res_in_runner)) {
                    print_exception(runner_ctx);
                    JS_FreeValue(cc->ctx, result);
                    return false;
                }
                std::string act = serialize_value(runner_ctx, res_in_runner);
                JS_FreeValue(runner_ctx, res_in_runner);
                if (exp != act) {
                    std::cerr << "result mismatch in " << tx.contract << "." << tx.method
                              << ": expected " << exp << ", got " << act << std::endl;
                    JS_FreeValue(cc->ctx, result);
                    return false;
                }
            }
            JS_FreeValue(cc->ctx, result);
        }
    }
    return true;
}

static bool compare_expected(JSContext* ctx, const TestCase& tc) {
    bool ok = true;
    for (auto& [key, guard] : tc.expected) {
        auto it = granules.find(key);
        if (it == granules.end()) {
            std::cerr << "  missing granule: " << key << std::endl;
            ok = false;
            continue;
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
    JSRuntime* rt = JS_NewRuntime();
    if (!rt) return 1;
    JSContext* ctx = JS_NewContext(rt);
    if (!ctx) { JS_FreeRuntime(rt); return 1; }

    g_runner_ctx = ctx;

    auto dir = list_directory("tx");
    int failures = 0;
    int total = 0;

    for (auto& file : dir) {
        if (file.size() < 6 || file.substr(file.size() - 6) != ".tx.js") continue;
        total++;
        std::string path = "tx/" + file;
        std::cout << "Running: " << path << std::endl;

        TestCase tc;
        if (!load_test(ctx, path, tc)) { failures++; continue; }

        bool load_ok = true;
        for (auto& tx : tc.transactions) {
            if (!get_contract(tx.contract, "contracts")) { load_ok = false; break; }
        }
        if (!load_ok) { std::cerr << "  cannot load contracts" << std::endl; failures++; continue; }

        apply_initial(ctx, tc);

        if (!run_transactions(ctx, tc)) { failures++; continue; }
        if (!compare_expected(ctx, tc)) { failures++; continue; }

        std::cout << "  PASS" << std::endl;
    }

    granules.clear();
    contract_cache.clear();

    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);

    std::cout << total - failures << "/" << total << " passed" << std::endl;
    return failures ? 1 : 0;
}