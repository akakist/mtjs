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
static CallContext* require_ctx(JSIContext* ctx) ;


void delete_prop_str(JSIContext* ctx, JSIValueConst obj, const char* name) {
    JSAtom atom = JSI_NewAtom(ctx, name);
    if (atom != JSI_ATOM_NULL) {
        JSI_DeleteProperty(ctx, obj, atom, 0);
        JSI_FreeAtom(ctx, atom);
    }
}

CallContext* get_call_context(JSIContext* ctx) {
    return (CallContext*)JSI_GetContextOpaque(ctx);
}

static CallContext* require_ctx(JSIContext* ctx) 
{
    CallContext* c = get_call_context(ctx);
    if (!c) JSI_ThrowTypeError(ctx, "no call context");
    return c;
}

// ---------- msg getters: читают из root t_params ----------
static JSIValue msg_sender(JSIContext* ctx, JSIValueConst, int, JSIValueConst*) {
  CallContext* c = require_ctx(ctx); if (!c) return JSI_EXCEPTION;
    if (!c->msg_source) return JSI_NewString(ctx, "");
    auto s = c->msg_source->get_sender();
    return JSI_NewStringLen(ctx, s.data(), s.size());
}
static JSIValue msg_value(JSIContext* ctx, JSIValueConst, int, JSIValueConst*) {
    CallContext* c = require_ctx(ctx); if (!c) return JSI_EXCEPTION;
    return JSI_NewInt64(ctx, c->msg_source ? c->msg_source->get_value() : 0);
}
static JSIValue msg_gasLimit(JSIContext* ctx, JSIValueConst, int, JSIValueConst*) {
    CallContext* c = require_ctx(ctx); if (!c) return JSI_EXCEPTION;
    return JSI_NewInt64(ctx, c->msg_source? c->msg_source->get_gasLimit() : 0);
}
static JSIValue msg_gasUsed(JSIContext* ctx, JSIValueConst, int, JSIValueConst*) {
    CallContext* c = require_ctx(ctx); if (!c) return JSI_EXCEPTION;
    return JSI_NewInt64(ctx, c->msg_source ? (int64_t)c->msg_source->get_gasUsed() : 0);
}
static JSIValue msg_blockTimestamp(JSIContext* ctx, JSIValueConst, int, JSIValueConst*) {
    CallContext* c = require_ctx(ctx); if (!c) return JSI_EXCEPTION;
    return JSI_NewInt64(ctx, c->msg_source ? (int64_t)c->msg_source->get_blockTimestamp() : 0);
}
static JSIValue msg_blockHeight(JSIContext* ctx, JSIValueConst, int, JSIValueConst*) {
    CallContext* c = require_ctx(ctx); if (!c) return JSI_EXCEPTION;
    return JSI_NewInt64(ctx, c->msg_source ? (int64_t)c->msg_source->get_blockHeight() : 0);
}
static JSIValue msg_nonce(JSIContext* ctx, JSIValueConst, int, JSIValueConst*) {
    CallContext* c = require_ctx(ctx); if (!c) return JSI_EXCEPTION;
    return JSI_NewInt64(ctx, c->msg_source ? (int64_t)c->msg_source->get_nonce() : 0);
}
static JSIValue msg_chainId(JSIContext* ctx, JSIValueConst, int, JSIValueConst*) {
    CallContext* c = require_ctx(ctx); if (!c) return JSI_EXCEPTION;
    return JSI_NewInt64(ctx, c->msg_source ? (int64_t)c->msg_source->get_chainId() : 0);
}
static JSIValue msg_txHash(JSIContext* ctx, JSIValueConst, int, JSIValueConst*) {
    CallContext* c = require_ctx(ctx); if (!c) return JSI_EXCEPTION;
    if (!c->msg_source) return JSI_NewString(ctx, "");
    auto hex = base16::encode(c->msg_source->get_txHash());
    return JSI_NewStringLen(ctx, hex.data(), hex.size());
}
static JSIValue msg_txIndex(JSIContext* ctx, JSIValueConst, int, JSIValueConst*) {
    CallContext* c = require_ctx(ctx); if (!c) return JSI_EXCEPTION;
    return JSI_NewInt64(ctx, c->msg_source ? (int64_t)c->msg_source->get_txIndex() : 0);
}
static JSIValue msg_caller(JSIContext* ctx, JSIValueConst, int, JSIValueConst*) {
    CallContext* c = require_ctx(ctx); if (!c) return JSI_EXCEPTION;
    return JSI_NewStringLen(ctx, c->caller.container.data(), c->caller.container.size());
}

JSIValue build_msg_object(JSIContext* ctx, const jsiMsg& /*m*/)
{
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
    } catch (const std::bad_alloc&) { return JSI_ThrowOutOfMemory(ctx); }
    catch (const std::exception& e) { return JSI_ThrowInternalError(ctx, "registerMethod: %s", e.what()); }
    catch (...) { return JSI_ThrowInternalError(ctx, "registerMethod: unknown C++ exception"); }
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
    } catch (const std::bad_alloc&) { return JSI_ThrowOutOfMemory(ctx); }
    catch (const std::exception& e) { return JSI_ThrowInternalError(ctx, "registerView: %s", e.what()); }
    catch (...) { return JSI_ThrowInternalError(ctx, "registerView: unknown C++ exception"); }
}

JSIValue jsi_createGranule(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    try {
        if (argc < 2 || !JSI_IsString(argv[0])) return JSI_ThrowTypeError(ctx, "createGranule(key, value[, ttl])");
        if (JSI_IsUndefined(argv[1])){
            return JSI_ThrowTypeError(ctx, "createGranule: undefined not allowed");
        } 
        CallContext* call = (CallContext*)JSI_GetContextOpaque(ctx);
        if (!call){
            return JSI_ThrowTypeError(ctx, "no call context");
        } 
        if (call->in_view) 
        {
            return JSI_ThrowTypeError(ctx, "createGranule is not allowed in view");
        }
        ContractContext* cc = call->contract.get();
        GranuleIO* io = call->host->granules.get();
        JSIScope<20,20> scope(ctx);
        auto key = scope.toStdString(argv[0]);
        if (key.empty()) return JSI_EXCEPTION;
        int64_t ttl = 0;
        if (argc >= 3 && !JSI_IsUndefined(argv[2])) {
            if (JSI_ToInt64(ctx, &ttl, argv[2]) != 0){
                return JSI_ThrowTypeError(ctx, "createGranule: ttl must be int64");
            }
            if (ttl < 0){
                 return JSI_ThrowTypeError(ctx, "createGranule: ttl must be >= 0");
            }
        }
        CONTRACT_DATA_id cid; cid.container=key;
        if (io->existsGranule(cc->name, cid))
        {
            return JSI_ThrowTypeError(ctx, "createGranule: already exists: %s%s", cc->name.container.c_str(), key.c_str());

        }
        std::string s = serialize_value(ctx, argv[1]);
        if (!is_serializable(s)){
            return JSI_ThrowTypeError(ctx, "createGranule(%s%s): cannot serialize value", cc->name.container.c_str(), key.c_str());
        } 
        if (s.size() > MAX_GRANULE_SIZE){
            return JSI_ThrowTypeError(ctx, "createGranule(%s%s): value too large", cc->name.container.c_str(), key.c_str());
        } 
        int64_t now = call->msg_source->get_blockTimestamp();
        if (auto err = io->createGranule(cc->name, cid, s, now, ttl, call->roll))
        {
            return JSI_ThrowTypeError(ctx, "createGranule(%s%s): %s", cc->name.container.c_str(), key.c_str(), err->c_str());

        }
        return JSI_UNDEFINED;
    } catch (const std::bad_alloc&) { return JSI_ThrowOutOfMemory(ctx); }
    catch (const std::exception& e) { return JSI_ThrowInternalError(ctx, "createGranule: %s", e.what()); }
    catch (...) { return JSI_ThrowInternalError(ctx, "createGranule: unknown C++ exception"); }
}

JSIValue jsi_setGranule(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    try {
        if (argc < 2 || !JSI_IsString(argv[0])) return JSI_ThrowTypeError(ctx, "setGranule(key, value)");
        if (JSI_IsUndefined(argv[1])) return JSI_ThrowTypeError(ctx, "setGranule: undefined not allowed");
        CallContext* call = (CallContext*)JSI_GetContextOpaque(ctx);
        if (!call) return JSI_ThrowTypeError(ctx, "no call context");
        if (call->in_view) return JSI_ThrowTypeError(ctx, "setGranule is not allowed in view");
        ContractContext* cc = call->contract.get();
        GranuleIO* io = call->host->granules.get();
        JSIScope<20,20> scope(ctx);
        auto key = scope.toStdString(argv[0]);
        if (key.empty()) return JSI_EXCEPTION;
        CONTRACT_DATA_id cid; cid.container=key;
        if (!io->existsGranule(cc->name,cid)) return JSI_ThrowTypeError(ctx, "setGranule: not found: %s%s", cc->name.container.c_str(), key.c_str());
        std::string s = serialize_value(ctx, argv[1]);
        if (!is_serializable(s)) return JSI_ThrowTypeError(ctx, "setGranule(%s%s): cannot serialize value", cc->name.container.c_str(), key.c_str());
        if (s.size() > MAX_GRANULE_SIZE) return JSI_ThrowTypeError(ctx, "setGranule(%s%s): value too large", cc->name.container.c_str(), key.c_str());
        if (auto err = io->setGranule(cc->name, cid, s, call->roll))
        {
            return JSI_ThrowTypeError(ctx, "setGranule(%s%s): %s", cc->name.container.c_str(), key.c_str(), err->c_str());
        }
        return JSI_UNDEFINED;
    } catch (const std::bad_alloc&) { return JSI_ThrowOutOfMemory(ctx); }
    catch (const std::exception& e) { return JSI_ThrowInternalError(ctx, "setGranule: %s", e.what()); }
    catch (...) { return JSI_ThrowInternalError(ctx, "setGranule: unknown C++ exception"); }
}

JSIValue jsi_getGranule(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    try {
        if (argc < 1 || !JSI_IsString(argv[0])){
            return JSI_ThrowTypeError(ctx, "getGranule(key)");
        }
         
        CallContext* call = (CallContext*)JSI_GetContextOpaque(ctx);
        if (!call){
            return JSI_ThrowTypeError(ctx, "no call context");
        } 
        ContractContext* cc = call->contract.get();
        GranuleIO* io = call->host->granules.get();
        JSIScope<20,20> scope(ctx);
        auto key = scope.toStdString(argv[0]);
        if (key.empty()){
             return JSI_EXCEPTION;
        }
        CONTRACT_DATA_id cid; cid.container=key;
        std::string value;
        if (!io->getGranule(cc->name,cid, value)){
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
    try {
        if (argc < 1 || !JSI_IsNumber(argv[0])) return JSI_ThrowTypeError(ctx, "consumeValue(amount)");
        CallContext* call = (CallContext*)JSI_GetContextOpaque(ctx);
        if (!call) return JSI_ThrowTypeError(ctx, "no call context");
        if (call->in_view) return JSI_ThrowTypeError(ctx, "consumeValue is not allowed in view");
        if (!call->msg_source) return JSI_ThrowTypeError(ctx, "consumeValue: no value context");
        if (!call->host.valid() || !call->host->granules.valid() || !call->contract.valid())
            return JSI_ThrowTypeError(ctx, "consumeValue: no contract/granules context");
        int64_t amount = 0;
        if (JSI_ToInt64(ctx, &amount, argv[0]) != 0) return JSI_ThrowTypeError(ctx, "consumeValue: amount must be int64");
        if (amount < 0) return JSI_ThrowTypeError(ctx, "consumeValue: amount < 0");
        int64_t remaining = call->msg_source->get_value();
        if (amount > remaining)
            return JSI_ThrowTypeError(ctx, "consumeValue: insufficient value (amount=%lld remaining=%lld)",
                                      (long long)amount, (long long)remaining);
        // value: sender -> contract
        call->msg_source->consume_value(amount);
        if (auto err = call->host->granules->addContractBalance(call->contract->name, amount, call->roll))
            return JSI_ThrowTypeError(ctx, "consumeValue: %s", err->c_str());
        return JSI_UNDEFINED;
    } catch (const std::bad_alloc&) { return JSI_ThrowOutOfMemory(ctx); }
    catch (const std::exception& e) { return JSI_ThrowInternalError(ctx, "consumeValue: %s", e.what()); }
    catch (...) { return JSI_ThrowInternalError(ctx, "consumeValue: unknown C++ exception"); }
}
JSIValue jsi_refundValue(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    try {
        if (argc < 1 || !JSI_IsNumber(argv[0])) return JSI_ThrowTypeError(ctx, "refundValue(amount)");
        CallContext* call = (CallContext*)JSI_GetContextOpaque(ctx);
        if (!call) return JSI_ThrowTypeError(ctx, "no call context");
        if (call->in_view) return JSI_ThrowTypeError(ctx, "refundValue is not allowed in view");
        if (!call->msg_source) return JSI_ThrowTypeError(ctx, "refundValue: no value context");
        if (!call->host.valid() || !call->host->granules.valid() || !call->contract.valid())
            return JSI_ThrowTypeError(ctx, "refundValue: no contract/granules context");
        int64_t amount = 0;
        if (JSI_ToInt64(ctx, &amount, argv[0]) != 0) return JSI_ThrowTypeError(ctx, "refundValue: amount must be int64");
        if (amount < 0) return JSI_ThrowTypeError(ctx, "refundValue: amount < 0");
        int64_t cur = 0;
        if (!call->host->granules->getContractBalance(call->contract->name, cur) || cur < amount)
            return JSI_ThrowTypeError(ctx, "refundValue: insufficient contract balance (amount=%lld balance=%lld)",
                                      (long long)amount, (long long)cur);
        // contract -> sender
        if (auto err = call->host->granules->addContractBalance(call->contract->name, -amount, call->roll))
            return JSI_ThrowTypeError(ctx, "refundValue: %s", err->c_str());
        call->msg_source->refund_value(amount);
        return JSI_UNDEFINED;
    } catch (const std::bad_alloc&) { return JSI_ThrowOutOfMemory(ctx); }
    catch (const std::exception& e) { return JSI_ThrowInternalError(ctx, "refundValue: %s", e.what()); }
    catch (...) { return JSI_ThrowInternalError(ctx, "refundValue: unknown C++ exception"); }
}

JSIValue jsi_remainingValue(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst*) {
    CallContext* call = (CallContext*)JSI_GetContextOpaque(ctx);
    if (!call) return JSI_ThrowTypeError(ctx, "no call context");
    if (!call->msg_source) return JSI_NewInt64(ctx, 0);
    return JSI_NewInt64(ctx, call->msg_source->get_value());
}

int interrupt_handler(JSIRuntime* rt, void* opaque) {
    (void)rt;
    CallContext* call = (CallContext*)opaque;
    if (!call || !call->msg_source) return 0;
    call->msg_source->consume_gas(GAS_STEP);
    int64_t gasUsed = call->msg_source->get_gasUsed();
    int64_t gasLimit = call->msg_source->get_gasLimit();
    if (gasUsed > gasLimit) return 1;   // прервать (exception)
    return 0;
}
#ifdef KALL
JSIValue jsi_consumeValue(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    try {
        if (argc < 1 || !JSI_IsNumber(argv[0])) return JSI_ThrowTypeError(ctx, "consumeValue(amount)");
        CallContext* call = (CallContext*)JSI_GetContextOpaque(ctx);
        if (!call) return JSI_ThrowTypeError(ctx, "no call context");
        if (call->in_view) return JSI_ThrowTypeError(ctx, "consumeValue is not allowed in view");
        if (!call->value_ptr) return JSI_ThrowTypeError(ctx, "consumeValue: no value context");
        int64_t amount = 0;
        if (JSI_ToInt64(ctx, &amount, argv[0]) != 0) return JSI_ThrowTypeError(ctx, "consumeValue: amount must be int64");
        if (amount < 0) return JSI_ThrowTypeError(ctx, "consumeValue: amount < 0");
        if (amount > *call->value_ptr)
            return JSI_ThrowTypeError(ctx, "consumeValue: insufficient value (amount=%lld remaining=%lld)",
                                      (long long)amount, (long long)*call->value_ptr);
        *call->value_ptr -= amount;
        return JSI_UNDEFINED;
    } catch (const std::bad_alloc&) { return JSI_ThrowOutOfMemory(ctx); }
    catch (const std::exception& e) { return JSI_ThrowInternalError(ctx, "consumeValue: %s", e.what()); }
    catch (...) { return JSI_ThrowInternalError(ctx, "consumeValue: unknown C++ exception"); }
}

JSIValue jsi_refundValue(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    try {
        if (argc < 1 || !JSI_IsNumber(argv[0])) return JSI_ThrowTypeError(ctx, "refundValue(amount)");
        CallContext* call = (CallContext*)JSI_GetContextOpaque(ctx);
        if (!call) return JSI_ThrowTypeError(ctx, "no call context");
        if (call->in_view) return JSI_ThrowTypeError(ctx, "refundValue is not allowed in view");
        if (!call->value_ptr) return JSI_ThrowTypeError(ctx, "refundValue: no value context");
        int64_t amount = 0;
        if (JSI_ToInt64(ctx, &amount, argv[0]) != 0) return JSI_ThrowTypeError(ctx, "refundValue: amount must be int64");
        if (amount < 0) return JSI_ThrowTypeError(ctx, "refundValue: amount < 0");
        *call->value_ptr += amount;
        return JSI_UNDEFINED;
    } catch (const std::bad_alloc&) { return JSI_ThrowOutOfMemory(ctx); }
    catch (const std::exception& e) { return JSI_ThrowInternalError(ctx, "refundValue: %s", e.what()); }
    catch (...) { return JSI_ThrowInternalError(ctx, "refundValue: unknown C++ exception"); }
}

JSIValue jsi_remainingValue(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst*) {
    CallContext* call = (CallContext*)JSI_GetContextOpaque(ctx);
    if (!call) return JSI_ThrowTypeError(ctx, "no call context");
    if (!call->value_ptr) return JSI_NewInt64(ctx, 0);
    return JSI_NewInt64(ctx, *call->value_ptr);
}
#endif
JSIValue jsi_emit(JSIContext* ctx, JSIValueConst, int argc, JSIValueConst* argv) {
    try {
        if (argc < 2 || !JSI_IsString(argv[0])) return JSI_ThrowTypeError(ctx, "emit(name, value)");
        CallContext* call = (CallContext*)JSI_GetContextOpaque(ctx);
        if (!call) return JSI_ThrowTypeError(ctx, "no call context");
        if (call->in_view) return JSI_ThrowTypeError(ctx, "emit is not allowed in view");
        ContractContext* cc = call->contract.get();
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
    try {
        if (argc < 3 || !JSI_IsString(argv[0]) || !JSI_IsString(argv[1]))
            return JSI_ThrowTypeError(ctx, "callMethod(contract, method, params)");
        CallContext* self = (CallContext*)JSI_GetContextOpaque(ctx);
        if (!self || !self->call_stack.valid() || !self->host.valid())
            return JSI_ThrowTypeError(ctx, "callMethod outside of a call");
        if (self->in_view)                                                         // ← guard
            return JSI_ThrowTypeError(ctx, "callMethod is not allowed in view");   // ← guard
        JSIScope<20,20> scope(ctx);
        CONTRACT_id contract_name; contract_name.container = scope.toStdString(argv[0]);
        auto method_name = scope.toStdString(argv[1]);
        if (contract_name.container.empty() || method_name.empty()) return JSI_EXCEPTION;
        if (self->contract->name == contract_name) return JSI_ThrowTypeError(ctx, "callMethod to self not allowed");
        auto cc = self->host->get_contract(contract_name);
        if (!cc.valid()) return JSI_ThrowTypeError(ctx, "contract not found: %s", contract_name.container.c_str());
        json params = json_from_jsi(ctx, argv[2]);
        void* prev = JSI_GetContextOpaque(cc->ctx);      // ContractContext* (или nullptr)
        JSI_SetContextOpaque(cc->ctx, self);             // CallContext* caller
        JSIValue r = cc->call_contract_method(method_name, params, self->msg_source, false,
                                               self->call_stack, self->host, self->roll, self->contract->name);
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
    try {
        if (argc < 3 || !JSI_IsString(argv[0]) || !JSI_IsString(argv[1]))
            return JSI_ThrowTypeError(ctx, "callView(contract, view, params)");
        CallContext* self = (CallContext*)JSI_GetContextOpaque(ctx);
        if (!self || !self->call_stack.valid() || !self->host.valid())
            return JSI_ThrowTypeError(ctx, "callView outside of a call");
        JSIScope<20,20> scope(ctx);
        CONTRACT_id contract_name; contract_name.container = scope.toStdString(argv[0]);
        auto view_name = scope.toStdString(argv[1]);
        if (contract_name.container.empty() || view_name.empty()) return JSI_EXCEPTION;
        if (self->contract->name == contract_name) return JSI_ThrowTypeError(ctx, "callView to self not allowed");
        auto cc = self->host->get_contract(contract_name);
        if (!cc.valid()) return JSI_ThrowTypeError(ctx, "contract not found: %s", contract_name.container.c_str());
        json params = json_from_jsi(ctx, argv[2]);
        void* prev = JSI_GetContextOpaque(cc->ctx);      // ContractContext* (или nullptr)
        JSI_SetContextOpaque(cc->ctx, self);             // CallContext* caller
        JSIValue r = cc->call_contract_method(view_name, params, self->msg_source, true,
                                               self->call_stack, self->host, self->roll, self->contract->name);
        JSI_SetContextOpaque(cc->ctx, prev);             // восстановить
        if (JSI_IsException(r)) { print_exception(cc->ctx); JSI_FreeValue(cc->ctx, r); return JSI_EXCEPTION; }
        JSIValue r_caller = convert_value(cc->ctx, ctx, r);
        JSI_FreeValue(cc->ctx, r);
        return r_caller;
    } catch (const std::bad_alloc&) { return JSI_ThrowOutOfMemory(ctx); }
    catch (const std::exception& e) { return JSI_ThrowInternalError(ctx, "callView: %s", e.what()); }
    catch (...) { return JSI_ThrowInternalError(ctx, "callView: unknown C++ exception"); }
}

void register_api(JSIContext* ctx) {
    JSIValue global = JSI_GetGlobalObject(ctx);
    JSI_SetPropertyStr(ctx, global, "registerMethod", JSI_NewCFunction(ctx, jsi_registerMethod, "registerMethod", 2));
    JSI_SetPropertyStr(ctx, global, "registerView",   JSI_NewCFunction(ctx, jsi_registerView,   "registerView", 2));
    JSI_SetPropertyStr(ctx, global, "createGranule",  JSI_NewCFunction(ctx, jsi_createGranule,  "createGranule", 3));
    JSI_SetPropertyStr(ctx, global, "setGranule",     JSI_NewCFunction(ctx, jsi_setGranule,     "setGranule", 2));
    JSI_SetPropertyStr(ctx, global, "getGranule",     JSI_NewCFunction(ctx, jsi_getGranule,     "getGranule", 1));
    JSI_SetPropertyStr(ctx, global, "emit",           JSI_NewCFunction(ctx, jsi_emit,           "emit", 2));
    JSI_SetPropertyStr(ctx, global, "callMethod",     JSI_NewCFunction(ctx, jsi_callMethod,     "callMethod", 3));
    JSI_SetPropertyStr(ctx, global, "callView",       JSI_NewCFunction(ctx, jsi_callView,       "callView", 3));
    JSI_SetPropertyStr(ctx, global, "consumeValue",   JSI_NewCFunction(ctx, jsi_consumeValue,   "consumeValue", 1));
    JSI_SetPropertyStr(ctx, global, "refundValue",    JSI_NewCFunction(ctx, jsi_refundValue,    "refundValue", 1));
    JSI_SetPropertyStr(ctx, global, "remainingValue", JSI_NewCFunction(ctx, jsi_remainingValue, "remainingValue", 0));
    JSI_FreeValue(ctx, global);
}


