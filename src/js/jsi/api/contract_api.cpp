#include "quickjs_i.h"
#include <iostream>
#include "ContractContext.h"
#include "contract_constants.h"
#include <vector>
#include <nlohmann/json.hpp>
#include "CallContext.h"
#include "jsiscope.h"
#include "convert.h"
#include "GranuleIO.h"
#include "ContractHost.h"
#include "contract_api.h"
#include "t_params.h"
#include "QUORUM.h"   // GAS_STEP

using json = nlohmann::json;
CallContext* require_ctx__1(JSIContext* ctx) ;


void delete_prop_str(JSIContext* ctx, JSIValueConst obj, const char* name) {
    MUTEX_INSPECTOR;
    JSAtom atom = JSI_NewAtom(ctx, name);
    if (atom != JSI_ATOM_NULL) {
        JSI_DeleteProperty(ctx, obj, atom, 0);
        JSI_FreeAtom(ctx, atom);
    }
}

CallContext* get_call_context(JSIContext* ctx) {
    return (CallContext*)JSI_GetContextOpaque(ctx);
}

CallContext* require_ctx__1(JSIContext* ctx) {
    auto* c = (CallContext*)JSI_GetContextOpaque(ctx);
    if (!c) {
        JSI_ThrowTypeError(ctx, "no call context");
        return nullptr;
    }
    return c;
}

CallContext* require_readonly_ctx(JSIContext* ctx) {
    auto* c = require_ctx__1(ctx);
    if (!c) {
        JSI_ThrowTypeError(ctx, "no call context");
        return nullptr;
    }
    return c;
}

CallContext* require_writable_ctx(JSIContext* ctx, bool need_msg_source) {
    auto* c = require_ctx__1(ctx);
    if (!c) {
        JSI_ThrowTypeError(ctx, "no call context");
        return nullptr;
    }
    if (c->in_view) {
        JSI_ThrowTypeError(ctx, "operation not allowed in view");
        return nullptr;
    }
    if (need_msg_source && !c->msg_source) {
        JSI_ThrowTypeError(ctx, "no msg source");
        return nullptr;
    }
    return c;
}

// ---------- msg getters: читают из root t_params ----------
static JSIValue msg_sender(JSIContext* ctx, JSIValueConst, int, JSIValueConst*) {
    MUTEX_INSPECTOR;
  CallContext* c = require_writable_ctx(ctx); if (!c) return JSI_EXCEPTION;
    if (!c->msg_source) return JSI_NewString(ctx, "");
    auto s = c->msg_source->get_sender();
    return JSI_NewStringLen(ctx, s.data(), s.size());
}
static JSIValue msg_value(JSIContext* ctx, JSIValueConst, int, JSIValueConst*) {
    MUTEX_INSPECTOR;
    CallContext* c = require_writable_ctx(ctx); if (!c) return JSI_EXCEPTION;
    return JSI_NewInt64(ctx, c->msg_source ? c->msg_source->get_value() : 0);
}
static JSIValue msg_gasLimit(JSIContext* ctx, JSIValueConst, int, JSIValueConst*) {
    MUTEX_INSPECTOR;
    CallContext* c = require_writable_ctx(ctx); if (!c) return JSI_EXCEPTION;
    return JSI_NewInt64(ctx, c->msg_source? c->msg_source->get_gasLimit() : 0);
}
static JSIValue msg_gasUsed(JSIContext* ctx, JSIValueConst, int, JSIValueConst*) {
    MUTEX_INSPECTOR;
    CallContext* c = require_writable_ctx(ctx); if (!c) return JSI_EXCEPTION;
    return JSI_NewInt64(ctx, c->msg_source ? (int64_t)c->msg_source->get_gasUsed() : 0);
}
static JSIValue msg_blockTimestamp(JSIContext* ctx, JSIValueConst, int, JSIValueConst*) {
    MUTEX_INSPECTOR;
    CallContext* c = require_writable_ctx(ctx); if (!c) return JSI_EXCEPTION;
    return JSI_NewInt64(ctx, c->msg_source ? (int64_t)c->msg_source->get_blockTimestamp() : 0);
}
static JSIValue msg_blockHeight(JSIContext* ctx, JSIValueConst, int, JSIValueConst*) {
    MUTEX_INSPECTOR;
    CallContext* c = require_writable_ctx(ctx); if (!c) return JSI_EXCEPTION;
    return JSI_NewInt64(ctx, c->msg_source ? (int64_t)c->msg_source->get_blockHeight() : 0);
}
static JSIValue msg_nonce(JSIContext* ctx, JSIValueConst, int, JSIValueConst*) {
    MUTEX_INSPECTOR;
    CallContext* c = require_writable_ctx(ctx); if (!c) return JSI_EXCEPTION;
    return JSI_NewInt64(ctx, c->msg_source ? (int64_t)c->msg_source->get_nonce() : 0);
}
static JSIValue msg_chainId(JSIContext* ctx, JSIValueConst, int, JSIValueConst*) {
    MUTEX_INSPECTOR;
    CallContext* c = require_writable_ctx(ctx); if (!c) return JSI_EXCEPTION;
    return JSI_NewInt64(ctx, c->msg_source ? (int64_t)c->msg_source->get_chainId() : 0);
}
static JSIValue msg_txHash(JSIContext* ctx, JSIValueConst, int, JSIValueConst*) {
    MUTEX_INSPECTOR;
    CallContext* c = require_writable_ctx(ctx); if (!c) return JSI_EXCEPTION;
    if (!c->msg_source) return JSI_NewString(ctx, "");
    auto hex = base16::encode(c->msg_source->get_txHash());
    return JSI_NewStringLen(ctx, hex.data(), hex.size());
}
static JSIValue msg_txIndex(JSIContext* ctx, JSIValueConst, int, JSIValueConst*) {
    MUTEX_INSPECTOR;
    CallContext* c = require_writable_ctx(ctx); if (!c) return JSI_EXCEPTION;
    return JSI_NewInt64(ctx, c->msg_source ? (int64_t)c->msg_source->get_txIndex() : 0);
}
static JSIValue msg_caller(JSIContext* ctx, JSIValueConst, int, JSIValueConst*) {
    MUTEX_INSPECTOR;
    CallContext* c = require_readonly_ctx(ctx); if (!c) return JSI_EXCEPTION;
    return JSI_NewStringLen(ctx, c->caller.container.data(), c->caller.container.size());
}

JSIValue build_msg_object(JSIContext* ctx, const jsiMsg& /*m*/)
{
    MUTEX_INSPECTOR;
    JSIValue obj = JSI_NewObject(ctx);
    auto def = [&](const char* name, JSIValue (*fn)(JSIContext*, JSIValueConst, int, JSIValueConst*)) {
        JSIValue getter = JSI_NewCFunction(ctx, fn, name, 0);
        JSI_DefinePropertyGetSet(ctx, obj, JSI_NewAtom(ctx, name), getter, JSI_UNDEFINED, JSI_PROP_ENUMERABLE | JSI_PROP_CONFIGURABLE);
    };
    def("sender", msg_sender);
    def("value", msg_value);
    def("gasLimit", msg_gasLimit);
    def("gasUsed", msg_gasUsed);
    def("blockTimestamp", msg_blockTimestamp);
    def("blockHeight", msg_blockHeight);
    def("nonce", msg_nonce);
    def("chainId", msg_chainId);
    def("txHash", msg_txHash);
    def("txIndex", msg_txIndex);
    def("caller", msg_caller);
    return obj;

}
JSIValue js_blake2b_hex(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv)
{
    MUTEX_INSPECTOR;
    JSIScope<10,10> scope(ctx);
    if (argc != 1 || !JSI_IsString(argv[0]))
        return JSI_ThrowTypeError(ctx, "blake2b_hex(str)");
    std::string s = scope.toStdString(argv[0]);
    auto h = blake2b_hash(s);
    auto hex = base16::encode(h.container);
    return JSI_NewStringLen(ctx, hex.data(), hex.size());
}
JSIValue jsi_registerMethod(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    MUTEX_INSPECTOR;
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
    } catch (const std::bad_alloc&) { return JSI_ThrowOutOfMemory(ctx); }
    catch (const std::exception& e) { return JSI_ThrowInternalError(ctx, "registerMethod: %s", e.what()); }
    catch (...) { return JSI_ThrowInternalError(ctx, "registerMethod: unknown C++ exception"); }
}

JSIValue jsi_registerView(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    MUTEX_INSPECTOR;
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
    } catch (const std::bad_alloc&) { return JSI_ThrowOutOfMemory(ctx); }
    catch (const std::exception& e) { return JSI_ThrowInternalError(ctx, "registerView: %s", e.what()); }
    catch (...) { return JSI_ThrowInternalError(ctx, "registerView: unknown C++ exception"); }
}

JSIValue jsi_createGranule(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    MUTEX_INSPECTOR;
    try {
        if (argc < 3 || !JSI_IsString(argv[0]))
            return JSI_ThrowTypeError(ctx, "createGranule(key, value, ttl)");
        if (JSI_IsUndefined(argv[1]))
            return JSI_ThrowTypeError(ctx, "createGranule: undefined not allowed");
        CallContext* call = require_writable_ctx(ctx);
        if (!call)
            return JSI_ThrowTypeError(ctx, "no call context");
        if (call->in_view)
            return JSI_ThrowTypeError(ctx, "createGranule is not allowed in view");
        // ▼▼▼
        if (!call->in_view && call->msg_source)
            call->msg_source->consume_gas(call->fees->gas_per_call + call->fees->gas_per_hostcall);
        // ▲▲▲            
        ContractContext* cc = call->contract;
        GranuleIO* io = call->host->granules.get();
        JSIScope<20,20> scope(ctx);
        auto key = scope.toStdString(argv[0]);
        if (key.empty())
            return JSI_EXCEPTION;

        // TTL: обязателен, > 0, <= MAX_GRANULE_TTL
        int64_t ttl = 0;
        if (JSI_ToInt64(ctx, &ttl, argv[2]) != 0)
            return JSI_ThrowTypeError(ctx, "createGranule: ttl must be int64");
        if (ttl <= 0)
            return JSI_ThrowTypeError(ctx, "createGranule: ttl must be > 0 (no eternal granules)");
        if (ttl > MAX_GRANULE_TTL)
            return JSI_ThrowTypeError(ctx, "createGranule: ttl > MAX_TTL (10 years)");
        int64_t ttl_gas = call->fees->gas_create_base
                        + (ttl + call->fees->ttl_seconds_per_gas - 1) / call->fees->ttl_seconds_per_gas;

        CONTRACT_DATA_id cid; cid.container = key;
        if (io->existsGranule(call->root, cc->name, cid))
            return JSI_ThrowTypeError(ctx, "createGranule: already exists: %s%s",
                                      cc->name.container.c_str(), key.c_str());
        std::string s = serialize_value(ctx, argv[1]);
        if (!is_serializable(s))
            return JSI_ThrowTypeError(ctx, "createGranule(%s%s): cannot serialize value",
                                      cc->name.container.c_str(), key.c_str());
        if (s.size() > MAX_GRANULE_SIZE)
            return JSI_ThrowTypeError(ctx, "createGranule(%s%s): value too large",
                                      cc->name.container.c_str(), key.c_str());
        int64_t now = call->msg_source->get_blockTimestamp();
        if (auto err = io->createGranule(call->root, cc->name, cid, s, now, ttl, call->roll))
            return JSI_ThrowTypeError(ctx, "createGranule(%s%s): %s",
                                      cc->name.container.c_str(), key.c_str(), err->c_str());

        // TTL газ — сендер
        call->msg_source->consume_gas(ttl_gas);

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
    MUTEX_INSPECTOR;
    try {
        if (argc < 2 || !JSI_IsString(argv[0]))
            return JSI_ThrowTypeError(ctx, "setGranule(key, value)");
        if (JSI_IsUndefined(argv[1]))
            return JSI_ThrowTypeError(ctx, "setGranule: undefined not allowed");
        CallContext* call = require_writable_ctx(ctx);
        if (!call)
            return JSI_ThrowTypeError(ctx, "no call context");
        if (call->in_view)
            return JSI_ThrowTypeError(ctx, "setGranule is not allowed in view");
        // ▼▼▼
        if (!call->in_view && call->msg_source)
            call->msg_source->consume_gas(call->fees->gas_per_call + call->fees->gas_per_hostcall);
        // ▲▲▲            
        ContractContext* cc = call->contract;
        GranuleIO* io = call->host->granules.get();
        JSIScope<20,20> scope(ctx);
        auto key = scope.toStdString(argv[0]);
        if (key.empty())
            return JSI_EXCEPTION;

        CONTRACT_DATA_id cid; cid.container = key;
        if (!io->existsGranule(call->root, cc->name, cid))
            return JSI_ThrowTypeError(ctx, "setGranule: not found: %s%s",
                                      cc->name.container.c_str(), key.c_str());

        int64_t ttl = io->getGranuleTTL(call->root, cc->name, cid);   // ← старый TTL
        if (ttl <= 0)
            return JSI_ThrowTypeError(ctx, "setGranule: no ttl (granule without ttl)");
        int64_t ttl_gas = (ttl + call->fees->ttl_seconds_per_gas - 1) / call->fees->ttl_seconds_per_gas;   // без base

        std::string s = serialize_value(ctx, argv[1]);
        if (!is_serializable(s))
            return JSI_ThrowTypeError(ctx, "setGranule(%s%s): cannot serialize value",
                                      cc->name.container.c_str(), key.c_str());
        if (s.size() > MAX_GRANULE_SIZE)
            return JSI_ThrowTypeError(ctx, "setGranule(%s%s): value too large",
                                      cc->name.container.c_str(), key.c_str());
        int64_t now = call->msg_source->get_blockTimestamp();
        if (auto err = io->setGranule(call->root, cc->name, cid, s, now, call->roll))
            return JSI_ThrowTypeError(ctx, "setGranule(%s%s): %s",
                                      cc->name.container.c_str(), key.c_str(), err->c_str());

        call->msg_source->consume_gas(ttl_gas);   // автопродление (сендер платит)
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
    MUTEX_INSPECTOR;
    try {
        if (argc < 1 || !JSI_IsString(argv[0])){
            return JSI_ThrowTypeError(ctx, "getGranule(key)");
        }
         
        CallContext* call = require_readonly_ctx(ctx);
        if (!call){
            return JSI_ThrowTypeError(ctx, "no call context");
        } 
        ContractContext* cc = call->contract;
        GranuleIO* io = call->host->granules.get();
        JSIScope<20,20> scope(ctx);
        auto key = scope.toStdString(argv[0]);
        if (key.empty()){
             return JSI_EXCEPTION;
        }
        CONTRACT_DATA_id cid; cid.container=key;
        std::string value;
        if (!io->getGranule(call->root, cc->name,cid, value)){
            return JSI_UNDEFINED;
        } 
        json j = json::parse(value);
        return json_to_jsi(ctx, j);
    } catch (const std::bad_alloc&) { return JSI_ThrowOutOfMemory(ctx); }
    catch (const json::exception& e) { 
        return JSI_ThrowTypeError(ctx, "getGranule: %s", e.what()); 
    }
    catch (const std::exception& e) { return JSI_ThrowInternalError(ctx, "getGranule: %s", e.what()); }
    catch (...) { return JSI_ThrowInternalError(ctx, "getGranule: unknown C++ exception"); }
}
JSIValue jsi_consumeValue(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    MUTEX_INSPECTOR;
    try {
        if (argc < 1 || !JSI_IsNumber(argv[0])) return JSI_ThrowTypeError(ctx, "consumeValue(amount)");
        CallContext* call = require_writable_ctx(ctx);
        if (!call) return JSI_ThrowTypeError(ctx, "no call context");
        if (call->in_view) return JSI_ThrowTypeError(ctx, "consumeValue is not allowed in view");
        if (!call->msg_source) return JSI_ThrowTypeError(ctx, "consumeValue: no value context");
        if (!call->host || !call->host->granules.valid() || !call->contract)
            return JSI_ThrowTypeError(ctx, "consumeValue: no contract/granules context");
        // газ — один раз
        call->msg_source->consume_gas(call->fees->gas_per_call + call->fees->gas_per_hostcall);
        int64_t amount = 0;
        if (JSI_ToInt64(ctx, &amount, argv[0]) != 0) return JSI_ThrowTypeError(ctx, "consumeValue: amount must be int64");
        if (amount < 0) return JSI_ThrowTypeError(ctx, "consumeValue: amount < 0");
        int64_t remaining = call->msg_source->get_value();
        if (amount > remaining)
            return JSI_ThrowTypeError(ctx, "consumeValue: insufficient value (amount=%lld remaining=%lld)",
                                      (long long)amount, (long long)remaining);
        call->msg_source->consume_value(amount);
        if (auto err = call->host->granules->addContractBalance(call->root, call->contract->name, amount, call->roll))
            return JSI_ThrowTypeError(ctx, "consumeValue: %s", err->c_str());
        {
            int64_t newbal = 0;
            call->host->granules->getContractBalance(call->root, call->contract->name, newbal);
            DBG(logErr2("consumeValue: contract=%s amount=%lld balance=%lld", call->contract->name.container.c_str(), (long long)amount, (long long)newbal));
        }
        return JSI_UNDEFINED;
    } catch (const std::bad_alloc&) { return JSI_ThrowOutOfMemory(ctx); }
    catch (const std::exception& e) { return JSI_ThrowInternalError(ctx, "consumeValue: %s", e.what()); }
    catch (...) { return JSI_ThrowInternalError(ctx, "consumeValue: unknown C++ exception"); }
}
JSIValue jsi_refundValue(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    MUTEX_INSPECTOR;
    try {
        if (argc < 1 || !JSI_IsNumber(argv[0])) return JSI_ThrowTypeError(ctx, "refundValue(amount)");
        CallContext* call = require_writable_ctx(ctx);
        if (!call) return JSI_ThrowTypeError(ctx, "no call context");
        if (call->in_view) return JSI_ThrowTypeError(ctx, "refundValue is not allowed in view");
        // ▼▼▼
        if (call->msg_source)
            call->msg_source->consume_gas(call->fees->gas_per_call + call->fees->gas_per_hostcall);
        // ▲▲▲        
        if (!call->msg_source) return JSI_ThrowTypeError(ctx, "refundValue: no value context");
        if (!call->host || !call->host->granules.valid() || !call->contract)
            return JSI_ThrowTypeError(ctx, "refundValue: no contract/granules context");
        int64_t amount = 0;
        if (JSI_ToInt64(ctx, &amount, argv[0]) != 0) return JSI_ThrowTypeError(ctx, "refundValue: amount must be int64");
        if (amount < 0) return JSI_ThrowTypeError(ctx, "refundValue: amount < 0");
        {
            int64_t cur = 0;
            call->host->granules->getContractBalance(call->root, call->contract->name, cur);
            logErr2("refundValue: contract=%s amount=%lld balance=%lld", call->contract->name.container.c_str(), (long long)amount, (long long)cur);            
        }
        int64_t cur = 0;
        if (!call->host->granules->getContractBalance(call->root, call->contract->name, cur) || cur < amount)
            return JSI_ThrowTypeError(ctx, "refundValue: insufficient contract balance (amount=%lld balance=%lld)",
                                      (long long)amount, (long long)cur);
        // contract -> sender
        if (auto err = call->host->granules->addContractBalance(call->root, call->contract->name, -amount, call->roll))
            return JSI_ThrowTypeError(ctx, "refundValue: %s", err->c_str());
        call->msg_source->refund_value(amount);
        return JSI_UNDEFINED;
    } catch (const std::bad_alloc&) { return JSI_ThrowOutOfMemory(ctx); }
    catch (const std::exception& e) { return JSI_ThrowInternalError(ctx, "refundValue: %s", e.what()); }
    catch (...) { return JSI_ThrowInternalError(ctx, "refundValue: unknown C++ exception"); }
}

JSIValue jsi_remainingValue(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst*) {
    MUTEX_INSPECTOR;
    CallContext* call = require_writable_ctx(ctx);
    if (!call) return JSI_ThrowTypeError(ctx, "no call context");
    if (!call->msg_source) return JSI_NewInt64(ctx, 0);
    return JSI_NewInt64(ctx, call->msg_source->get_value());
}

int interrupt_handler(JSIRuntime* rt, void* opaque) {
    MUTEX_INSPECTOR;
    (void)rt;
    CallContext* call = (CallContext*)opaque;
    if (!call || !call->msg_source) return 0;
    call->msg_source->consume_gas(call->fees->gas_per_jsi_step);
    int64_t gasUsed = call->msg_source->get_gasUsed();
    int64_t gasLimit = call->msg_source->get_gasLimit();
    if (gasUsed > gasLimit) return 1;   // прервать (exception)
    return 0;
}
JSIValue jsi_emit(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
     MUTEX_INSPECTOR;
   try {
        if (argc < 2 || !JSI_IsString(argv[0])) return JSI_ThrowTypeError(ctx, "emit(name, value)");
        CallContext* call = require_writable_ctx(ctx);
        if (!call) return JSI_ThrowTypeError(ctx, "no call context");
        if (call->in_view) return JSI_ThrowTypeError(ctx, "emit is not allowed in view");
        // ▼▼▼
        if (!call->in_view && call->msg_source)
            call->msg_source->consume_gas(call->fees->gas_per_call + call->fees->gas_per_hostcall);
        // ▲▲▲        
        ContractContext* cc = call->contract;
        JSIScope<20,20> scope(ctx);
        auto name = scope.toStdString(argv[0]);
        if (name.empty()) return JSI_EXCEPTION;
        std::string s = serialize_value(ctx, argv[1]);
        if (!is_serializable(s)) return JSI_ThrowTypeError(ctx, "emit(%s): cannot serialize value", name.c_str());
        call->emits.push_back(jsiEvent{ cc->name.container, name, s });
        return JSI_UNDEFINED;
    } catch (const std::bad_alloc&) { return JSI_ThrowOutOfMemory(ctx); }
    catch (const std::exception& e) { return JSI_ThrowInternalError(ctx, "emit: %s", e.what()); }
    catch (...) { return JSI_ThrowInternalError(ctx, "emit: unknown C++ exception"); }
}

JSIValue jsi_callMethod(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    MUTEX_INSPECTOR;
    try {
        if (argc < 3 || !JSI_IsString(argv[0]) || !JSI_IsString(argv[1]))
            return JSI_ThrowTypeError(ctx, "callMethod(contract, method, params)");
        CallContext* call = require_writable_ctx(ctx);
        if (!call) return JSI_EXCEPTION;   // exception уже выставлен (in_view / no ctx / no msg_source)
        if (!call->call_stack || !call->host)
            return JSI_ThrowTypeError(ctx, "callMethod outside of a call");
        // ▼▼▼
        if (call->msg_source)
            call->msg_source->consume_gas(call->fees->gas_per_call + call->fees->gas_per_hostcall);
        JSIScope<20,20> scope(ctx);
        CONTRACT_id contract_name; contract_name.container = scope.toStdString(argv[0]);
        auto method_name = scope.toStdString(argv[1]);
        if (contract_name.container.empty() || method_name.empty()) return JSI_EXCEPTION;
        if (call->contract->name == contract_name) return JSI_ThrowTypeError(ctx, "callMethod to self not allowed");
        auto cc = call->host->get_contract(call->root,contract_name);
        if (!cc.valid()) return JSI_ThrowTypeError(ctx, "contract not found: %s", contract_name.container.c_str());
        json params = json_from_jsi(ctx, argv[2]);
        void* prev = JSI_GetContextOpaque(cc->ctx);      // ContractContext* (или nullptr)
        JSI_SetContextOpaque(cc->ctx, call);             // CallContext* caller
        JSIValue r = cc->call_contract_method(call->root, method_name, params, call->msg_source, 
                                               call->call_stack, call->host, call->roll, call->contract->name,call->fees,call->deleted_granules);
        JSI_SetContextOpaque(cc->ctx, prev);             // восстановить
        if (JSI_IsException(r)) { print_exception(cc->ctx); JSI_FreeValue(cc->ctx, r); return JSI_EXCEPTION; }
        JSIValue r_caller = convert_value(cc->ctx, ctx, r);
        JSI_FreeValue(cc->ctx, r);
        return r_caller;
    } catch (const std::bad_alloc&) { return JSI_ThrowOutOfMemory(ctx); }
    catch (const std::exception& e) { return JSI_ThrowInternalError(ctx, "callMethod: %s", e.what()); }
    catch (...) { return JSI_ThrowInternalError(ctx, "callMethod: unknown C++ exception"); }
}

JSIValue jsi_callView(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    MUTEX_INSPECTOR;
    try {
        if (argc < 4 || !JSI_IsString(argv[0]) || !JSI_IsString(argv[1]))
            return JSI_ThrowTypeError(ctx, "callView(contract, view, params, timeout_sec)");
        CallContext* call = require_readonly_ctx(ctx);
        if (!call || !call->call_stack || !call->host)
            return JSI_ThrowTypeError(ctx, "callView outside of a call");
        if (call->in_view)
            return JSI_ThrowTypeError(ctx, "callView is not allowed in view");
        JSIScope<20,20> scope(ctx);
        CONTRACT_id contract_name; contract_name.container = scope.toStdString(argv[0]);
        auto view_name = scope.toStdString(argv[1]);
        if (contract_name.container.empty() || view_name.empty()) return JSI_EXCEPTION;
        if (call->contract->name == contract_name) return JSI_ThrowTypeError(ctx, "callView to self not allowed");
        int64_t timeout_sec = 5;
        if (!JSI_IsUndefined(argv[3])) {
            if (JSI_ToInt64(ctx, &timeout_sec, argv[3]) != 0)
                return JSI_ThrowTypeError(ctx, "callView: timeout_sec must be int64");
        }
        auto cc = call->host->get_contract(call->root, contract_name);
        if (!cc.valid()) return JSI_ThrowTypeError(ctx, "contract not found: %s", contract_name.container.c_str());
        json params = json_from_jsi(ctx, argv[2]);
        void* prev = JSI_GetContextOpaque(cc->ctx);
        JSI_SetContextOpaque(cc->ctx, call);
        JSIValue r = cc->call_contract_view(call->root, view_name, params, call->call_stack, call->host, timeout_sec);
        JSI_SetContextOpaque(cc->ctx, prev);
        if (JSI_IsException(r)) { print_exception(cc->ctx); JSI_FreeValue(cc->ctx, r); return JSI_EXCEPTION; }
        JSIValue r_caller = convert_value(cc->ctx, ctx, r);
        JSI_FreeValue(cc->ctx, r);
        return r_caller;
    } catch (const std::bad_alloc&) { return JSI_ThrowOutOfMemory(ctx); }
    catch (const std::exception& e) { return JSI_ThrowInternalError(ctx, "callView: %s", e.what()); }
    catch (...) { return JSI_ThrowInternalError(ctx, "callView: unknown C++ exception"); }
}

JSIValue jsi_deleteGranule(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    MUTEX_INSPECTOR;
    try {
        if (argc < 1 || !JSI_IsString(argv[0]))
            return JSI_ThrowTypeError(ctx, "deleteGranule(key)");
        CallContext* call = require_writable_ctx(ctx);
        if (!call) return JSI_ThrowTypeError(ctx, "no call context");
        if (call->in_view) return JSI_ThrowTypeError(ctx, "deleteGranule is not allowed in view");
        // ▼▼▼
        if (!call->in_view && call->msg_source)
            call->msg_source->consume_gas(call->fees->gas_per_call + call->fees->gas_per_hostcall);
        // ▲▲▲        
        ContractContext* cc = call->contract;
        GranuleIO* io = call->host->granules.get();
        JSIScope<20,20> scope(ctx);
        auto key = scope.toStdString(argv[0]);
        if (key.empty()) return JSI_EXCEPTION;
        CONTRACT_DATA_id cid; cid.container = key;
        std::string value;
        if (!io->getGranule(call->root, cc->name, cid, value))
            return JSI_ThrowTypeError(ctx, "deleteGranule: not found: %s", key.c_str());
        if (auto err = io->deleteGranule(call->root, cc->name, cid, call->roll, call->deleted_granules))
            return JSI_ThrowTypeError(ctx, "deleteGranule: %s", err->c_str());
        return JSI_UNDEFINED;
    } catch (const std::bad_alloc&) { return JSI_ThrowOutOfMemory(ctx); }
    catch (const std::exception& e) { return JSI_ThrowInternalError(ctx, "deleteGranule: %s", e.what()); }
    catch (...) { return JSI_ThrowInternalError(ctx, "deleteGranule: unknown C++ exception"); }
}
void register_api(JSIContext* ctx) {
    MUTEX_INSPECTOR;
    JSIValue global = JSI_GetGlobalObject(ctx);
    JSI_SetPropertyStr(ctx, global, "registerMethod", JSI_NewCFunction(ctx, jsi_registerMethod, "registerMethod", 2));
    JSI_SetPropertyStr(ctx, global, "registerView",   JSI_NewCFunction(ctx, jsi_registerView,   "registerView", 2));
    JSI_SetPropertyStr(ctx, global, "createGranule",  JSI_NewCFunction(ctx, jsi_createGranule,  "createGranule", 3));
    JSI_SetPropertyStr(ctx, global, "setGranule",     JSI_NewCFunction(ctx, jsi_setGranule,     "setGranule", 2));
    JSI_SetPropertyStr(ctx, global, "getGranule",     JSI_NewCFunction(ctx, jsi_getGranule,     "getGranule", 1));
    JSI_SetPropertyStr(ctx, global, "emit",           JSI_NewCFunction(ctx, jsi_emit,           "emit", 2));
    JSI_SetPropertyStr(ctx, global, "callMethod",     JSI_NewCFunction(ctx, jsi_callMethod,     "callMethod", 3));
    JSI_SetPropertyStr(ctx, global, "consumeValue",   JSI_NewCFunction(ctx, jsi_consumeValue,   "consumeValue", 1));
    JSI_SetPropertyStr(ctx, global, "refundValue",    JSI_NewCFunction(ctx, jsi_refundValue,    "refundValue", 1));
    JSI_SetPropertyStr(ctx, global, "remainingValue", JSI_NewCFunction(ctx, jsi_remainingValue, "remainingValue", 0));
    JSI_SetPropertyStr(ctx, global, "deleteGranule",  JSI_NewCFunction(ctx, jsi_deleteGranule, "deleteGranule", 1));
    JSI_SetPropertyStr(ctx, global, "callView",       JSI_NewCFunction(ctx, jsi_callView,       "callView", 4));
    JSI_FreeValue(ctx, global);
    register_storage_api(ctx);
}


