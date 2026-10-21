
#include "quickjs_i.h"
#include <iostream>
#include "ContractContext.h"
#include "contract_constants.h"
#include <vector>
#include <nlohmann/json.hpp>
#include "CallContext.h"
#include "jsiscope.h"
using json = nlohmann::json;
JSIValue build_msg_object(JSIContext* ctx, const jsiMsg& m);

void print_exception(JSIContext* ctx) 
{
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


std::string serialize_value(JSIContext* ctx, JSIValueConst val, int depth = 0) 
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
        JSPropertyEnum* props = nullptr;
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
#include "jsiMsg.h"
#include "CallStack.h"
#include "ContractHost.h"
#include "CallFrame.h"
JSIValue ContractContext::call_contract_method(const std::string& method_name,
                             JSIValueConst fn, JSIValueConst params_js,
                             const jsiMsg& m, bool is_view,
                             const REF_getter<CallStack>& call_stack,
                             const REF_getter<ContractHost>& host)
{
    if (call_stack->call_stack.size() >= (size_t)MAX_CALL_DEPTH)
        return JSI_ThrowTypeError(ctx, "call stack too deep (%zu >= %d)",
                                  call_stack->call_stack.size(), MAX_CALL_DEPTH);

    REF_getter<CallFrame> frame = new CallFrame;
    frame->contract = this;
    frame->method = method_name;
    frame->in_view = is_view;
    call_stack->call_stack.push_back(frame);

    CallContext call_ctx;
    call_ctx.contract = this;
    call_ctx.in_view = is_view;
    call_ctx.msg = m;
    call_ctx.depth = (int)call_stack->call_stack.size() - 1;
    call_ctx.call_stack = call_stack;
    call_ctx.host = host;

    void* saved = JSI_GetContextOpaque(ctx);
    JSI_SetContextOpaque(ctx, &call_ctx);

    JSI_UpdateStackTop(rt);

    JSIValue msg_obj = build_msg_object(ctx, m);
    JSIValue argv[2] = { params_js, msg_obj };
    int nargs = is_view ? 1 : 2;

    JSIValue r;
    try {
        r = JSI_Call(ctx, fn, JSI_UNDEFINED, nargs, argv);
    } catch (...) {
        JSI_SetContextOpaque(ctx, saved);
        JSI_FreeValue(ctx, msg_obj);
        call_stack->call_stack.pop_back();
        throw;
    }

    assert(JSI_GetContextOpaque(cc->ctx) == &call_ctx
           && "opaque was tampered inside JSI_Call");
    JSI_SetContextOpaque(ctx, saved);
    JSI_FreeValue(ctx, msg_obj);
    call_stack->call_stack.pop_back();
    return r;
}

JSIValue build_msg_object(JSIContext* ctx, const jsiMsg& m) {
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


JSIValue jsi_registerMethod(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    try {
        if (argc < 2 || !JSI_IsString(argv[0]) || !JSI_IsFunction(ctx, argv[1]))
            return JSI_ThrowTypeError(ctx, "registerMethod(name, fn)");
        ContractContext* cc = (ContractContext*)JSI_GetContextOpaque(ctx);
        if (!cc) return JSI_ThrowTypeError(ctx, "no contract context");
        JSIScope<20,20> scope(ctx);
        auto name = scope.toStdString(argv[0]);
        if (name.empty()) return JSI_EXCEPTION;
        cc->methods[name] = JSIValueGuard(ctx, JSI_DupValue(ctx, argv[1]));
        return JSI_UNDEFINED;
    } catch (const std::bad_alloc&) {
        return JSI_ThrowOutOfMemory(ctx);
    } catch (const std::exception& e) {
        return JSI_ThrowInternalError(ctx, "registerMethod: %s", e.what());
    } catch (...) {
        return JSI_ThrowInternalError(ctx, "registerMethod: unknown C++ exception");
    }
}

JSIValue jsi_registerView(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    try {
        if (argc < 2 || !JSI_IsString(argv[0]) || !JSI_IsFunction(ctx, argv[1]))
            return JSI_ThrowTypeError(ctx, "registerView(name, fn)");
        ContractContext* cc = (ContractContext*)JSI_GetContextOpaque(ctx);
        if (!cc) return JSI_ThrowTypeError(ctx, "no contract context");
        JSIScope<20,20> scope(ctx);
        auto name = scope.toStdString(argv[0]);
        if (name.empty()) return JSI_EXCEPTION;
        cc->views[name] = JSIValueGuard(ctx, JSI_DupValue(ctx, argv[1]));
        return JSI_UNDEFINED;
    } catch (const std::bad_alloc&) {
        return JSI_ThrowOutOfMemory(ctx);
    } catch (const std::exception& e) {
        return JSI_ThrowInternalError(ctx, "registerView: %s", e.what());
    } catch (...) {
        return JSI_ThrowInternalError(ctx, "registerView: unknown C++ exception");
    }
}

JSIValue jsi_createGranule(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    try {
        if (argc < 2 || !JSI_IsString(argv[0]))
            return JSI_ThrowTypeError(ctx, "createGranule(key, value[, ttl])");
        if (JSI_IsUndefined(argv[1]))
            return JSI_ThrowTypeError(ctx, "createGranule: undefined not allowed");
        CallContext* call = (CallContext*)JSI_GetContextOpaque(ctx);
        if (!call) return JSI_ThrowTypeError(ctx, "no call context");
        if (call->in_view)
            return JSI_ThrowTypeError(ctx, "createGranule is not allowed in view");
        ContractContext* cc = call->contract.get();
        GranuleIO* io = call->host->granules.get();

        JSIScope<20,20> scope(ctx);
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
        if (io->exists(full_key))
            return JSI_ThrowTypeError(ctx, "createGranule: already exists: %s", full_key.c_str());

        std::string s = serialize_value(ctx, argv[1]);
        if (!is_serializable(s))
            return JSI_ThrowTypeError(ctx, "createGranule(%s): cannot serialize value", full_key.c_str());
        if (s.size() > MAX_GRANULE_SIZE)
            return JSI_ThrowTypeError(ctx, "createGranule(%s): value too large (%zu > %zu)",
                                     full_key.c_str(), s.size(), MAX_GRANULE_SIZE);

        int64_t now = call->host->clock->current_time();
        if (!io->create(full_key, s, now, ttl))
            return JSI_ThrowTypeError(ctx, "createGranule: create failed: %s", full_key.c_str());
        return JSI_UNDEFINED;
    } catch (const std::bad_alloc&) {
        return JSI_ThrowOutOfMemory(ctx);
    } catch (const std::exception& e) {
        return JSI_ThrowInternalError(ctx, "createGranule: %s", e.what());
    } catch (...) {
        return JSI_ThrowInternalError(ctx, "createGranule: unknown C++ exception");
    }
}

JSIValue jsi_setGranule(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    try {
        if (argc < 2 || !JSI_IsString(argv[0]))
            return JSI_ThrowTypeError(ctx, "setGranule(key, value)");
        if (JSI_IsUndefined(argv[1]))
            return JSI_ThrowTypeError(ctx, "setGranule: undefined not allowed");
        CallContext* call = (CallContext*)JSI_GetContextOpaque(ctx);
        if (!call) return JSI_ThrowTypeError(ctx, "no call context");
        if (call->in_view)
            return JSI_ThrowTypeError(ctx, "setGranule is not allowed in view");
        ContractContext* cc = call->contract.get();
        GranuleIO* io = call->host->granules.get();

        JSIScope<20,20> scope(ctx);
        auto key = scope.toStdString(argv[0]);
        if (key.empty()) return JSI_EXCEPTION;

        std::string full_key = cc->name + "/" + key;
        if (!io->exists(full_key))
            return JSI_ThrowTypeError(ctx, "setGranule: not found: %s", full_key.c_str());

        std::string s = serialize_value(ctx, argv[1]);
        if (!is_serializable(s))
            return JSI_ThrowTypeError(ctx, "setGranule(%s): cannot serialize value", full_key.c_str());
        if (s.size() > MAX_GRANULE_SIZE)
            return JSI_ThrowTypeError(ctx, "setGranule(%s): value too large (%zu > %zu)",
                                     full_key.c_str(), s.size(), MAX_GRANULE_SIZE);

        if (!io->set(full_key, s))
            return JSI_ThrowTypeError(ctx, "setGranule: set failed: %s", full_key.c_str());
        return JSI_UNDEFINED;
    } catch (const std::bad_alloc&) {
        return JSI_ThrowOutOfMemory(ctx);
    } catch (const std::exception& e) {
        return JSI_ThrowInternalError(ctx, "setGranule: %s", e.what());
    } catch (...) {
        return JSI_ThrowInternalError(ctx, "setGranule: unknown C++ exception");
    }
}

JSIValue jsi_getGranule(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    try {
        if (argc < 1 || !JSI_IsString(argv[0]))
            return JSI_ThrowTypeError(ctx, "getGranule(key)");
        CallContext* call = (CallContext*)JSI_GetContextOpaque(ctx);
        if (!call) return JSI_ThrowTypeError(ctx, "no call context");
        ContractContext* cc = call->contract.get();
        GranuleIO* io = call->host->granules.get();

        JSIScope<20,20> scope(ctx);
        auto key = scope.toStdString(argv[0]);
        if (key.empty()) return JSI_EXCEPTION;
        std::string full_key = cc->name + "/" + key;
        std::string value;
        if (!io->get(full_key, value)) return JSI_UNDEFINED;
        json j = json::parse(value);
        return json_to_jsi(ctx, j);
    } catch (const std::bad_alloc&) {
        return JSI_ThrowOutOfMemory(ctx);
    } catch (const json::exception& e) {
        return JSI_ThrowTypeError(ctx, "getGranule: %s", e.what());
    } catch (const std::exception& e) {
        return JSI_ThrowInternalError(ctx, "getGranule: %s", e.what());
    } catch (...) {
        return JSI_ThrowInternalError(ctx, "getGranule: unknown C++ exception");
    }
}

JSIValue jsi_emit(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    try {
        if (argc < 2 || !JSI_IsString(argv[0]))
            return JSI_ThrowTypeError(ctx, "emit(name, value)");
        CallContext* call = (CallContext*)JSI_GetContextOpaque(ctx);
        if (!call) return JSI_ThrowTypeError(ctx, "no call context");
        if (call->in_view)
            return JSI_ThrowTypeError(ctx, "emit is not allowed in view");
        ContractContext* cc = call->contract.get();
        EventSink* sink = call->host->events.get();

        JSIScope<20,20> scope(ctx);
        auto name = scope.toStdString(argv[0]);
        if (name.empty()) return JSI_EXCEPTION;
        std::string s = serialize_value(ctx, argv[1]);
        if (!is_serializable(s))
            return JSI_ThrowTypeError(ctx, "emit(%s): cannot serialize value", name.c_str());
        sink->emit(cc->name, name, s);
        return JSI_UNDEFINED;
    } catch (const std::bad_alloc&) {
        return JSI_ThrowOutOfMemory(ctx);
    } catch (const std::exception& e) {
        return JSI_ThrowInternalError(ctx, "emit: %s", e.what());
    } catch (...) {
        return JSI_ThrowInternalError(ctx, "emit: unknown C++ exception");
    }
}

JSIValue jsi_callMethod(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    try {
        if (argc < 3 || !JSI_IsString(argv[0]) || !JSI_IsString(argv[1]))
            return JSI_ThrowTypeError(ctx, "callMethod(contract, method, params)");
        CallContext* self = (CallContext*)JSI_GetContextOpaque(ctx);
        if (!self || !self->call_stack.valid() || !self->host.valid())
            return JSI_ThrowTypeError(ctx, "callMethod outside of a call");

        JSIScope<20,20> scope(ctx);
        auto contract_name = scope.toStdString(argv[0]);
        auto method_name   = scope.toStdString(argv[1]);
        if (contract_name.empty() || method_name.empty()) return JSI_EXCEPTION;

        if (self->contract->name == contract_name)
            return JSI_ThrowTypeError(ctx, "callMethod to self not allowed");

        auto cc = self->host->get_contract(contract_name);
        if (!cc.valid())
            return JSI_ThrowTypeError(ctx, "contract not found: %s", contract_name.c_str());
        auto mit = cc->methods.find(method_name);
        if (mit == cc->methods.end())
            return JSI_ThrowTypeError(ctx, "method not found: %s.%s",
                                     contract_name.c_str(), method_name.c_str());

        JSIValue params_in_cc = convert_value(ctx, cc->ctx, argv[2]);
        if (JSI_IsException(params_in_cc)) return JSI_EXCEPTION;

        jsiMsg empty;
        JSIValue r = cc->call_contract_method(method_name, mit->second.get(),
                                          params_in_cc, empty, false,
                                          self->call_stack, self->host);
        JSI_FreeValue(cc->ctx, params_in_cc);

        if (JSI_IsException(r)) {
            print_exception(cc->ctx);
            JSI_FreeValue(cc->ctx, r);
            return JSI_EXCEPTION;
        }
        JSIValue r_caller = convert_value(cc->ctx, ctx, r);
        JSI_FreeValue(cc->ctx, r);
        return r_caller;
    } catch (const std::bad_alloc&) {
        return JSI_ThrowOutOfMemory(ctx);
    } catch (const std::exception& e) {
        return JSI_ThrowInternalError(ctx, "callMethod: %s", e.what());
    } catch (...) {
        return JSI_ThrowInternalError(ctx, "callMethod: unknown C++ exception");
    }
}

JSIValue jsi_callView(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    try {
        if (argc < 3 || !JSI_IsString(argv[0]) || !JSI_IsString(argv[1]))
            return JSI_ThrowTypeError(ctx, "callView(contract, view, params)");
        CallContext* self = (CallContext*)JSI_GetContextOpaque(ctx);
        if (!self || !self->call_stack.valid() || !self->host.valid())
            return JSI_ThrowTypeError(ctx, "callView outside of a call");

        JSIScope<20,20> scope(ctx);
        auto contract_name = scope.toStdString(argv[0]);
        auto view_name     = scope.toStdString(argv[1]);
        if (contract_name.empty() || view_name.empty()) return JSI_EXCEPTION;

        if (self->contract->name == contract_name)
            return JSI_ThrowTypeError(ctx, "callView to self not allowed");

        auto cc = self->host->get_contract(contract_name);
        if (!cc.valid())
            return JSI_ThrowTypeError(ctx, "contract not found: %s", contract_name.c_str());
        auto vit = cc->views.find(view_name);
        if (vit == cc->views.end())
            return JSI_ThrowTypeError(ctx, "view not found: %s.%s",
                                     contract_name.c_str(), view_name.c_str());

        JSIValue params_in_cc = convert_value(ctx, cc->ctx, argv[2]);
        if (JSI_IsException(params_in_cc)) return JSI_EXCEPTION;

        jsiMsg empty;
        JSIValue r = cc->call_contract_method(view_name, vit->second.get(),
                                          params_in_cc, empty, true,
                                          self->call_stack, self->host);
        JSI_FreeValue(cc->ctx, params_in_cc);

        if (JSI_IsException(r)) {
            print_exception(cc->ctx);
            JSI_FreeValue(cc->ctx, r);
            return JSI_EXCEPTION;
        }
        JSIValue r_caller = convert_value(cc->ctx, ctx, r);
        JSI_FreeValue(cc->ctx, r);
        return r_caller;
    } catch (const std::bad_alloc&) {
        return JSI_ThrowOutOfMemory(ctx);
    } catch (const std::exception& e) {
        return JSI_ThrowInternalError(ctx, "callView: %s", e.what());
    } catch (...) {
        return JSI_ThrowInternalError(ctx, "callView: unknown C++ exception");
    }
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
REF_getter<ContractContext> ContractHost::load_contract(const std::string& name) {
    auto it = contract_cache.find(name);
    if (it != contract_cache.end()) return it->second;

    std::string src;
    if (!source->load(name, src)) {
        return nullptr;
    }

    REF_getter<ContractContext> cc = new ContractContext();
    cc->name = name;

    if (!cc->create__()) {
        std::cerr << "cannot create context for " << name << std::endl;
        return nullptr;
    }

    register_api(cc->ctx);

    JSIValue global = JSI_GetGlobalObject(cc->ctx);
    const char* banned[] = {
        "Math","Date","RegExp","JSON","Proxy","Promise","Map","Set",
        "WeakRef","FinalizationRegistry","Atomics","SharedArrayBuffer"
    };
    for (auto b : banned) delete_prop_str(cc->ctx, global, b);
    JSI_FreeValue(cc->ctx, global);

    JSIValue compiled = JSI_Eval(cc->ctx, src.data(), src.size(), name.c_str(),
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

REF_getter<ContractContext> ContractHost::get_contract(const std::string& name) {
    auto it = contract_cache.find(name);
    if (it != contract_cache.end()) return it->second;
    return load_contract(name);
}
