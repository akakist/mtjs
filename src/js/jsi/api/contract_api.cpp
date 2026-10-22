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
using json = nlohmann::json;

void delete_prop_str(JSIContext* ctx, JSIValueConst obj, const char* name) {
    JSAtom atom = JSI_NewAtom(ctx, name);
    if (atom != JSI_ATOM_NULL) {
        JSI_DeleteProperty(ctx, obj, atom, 0);
        JSI_FreeAtom(ctx, atom);
    }
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

        CONTRACT_DATA_id cid;
        cid.container=key;

        if (io->exists(cc->name, cid))
            return JSI_ThrowTypeError(ctx, "createGranule: already exists: %s%s", cc->name.container.c_str(),key.c_str());

        std::string s = serialize_value(ctx, argv[1]);
        if (!is_serializable(s))
            return JSI_ThrowTypeError(ctx, "createGranule(%s%s): cannot serialize value", cc->name.container.c_str(),key.c_str());
        if (s.size() > MAX_GRANULE_SIZE)
            return JSI_ThrowTypeError(ctx, "createGranule(%s%s): value too large (%zu > %zu)",
                                     cc->name.container.c_str(),key.c_str(), s.size(), MAX_GRANULE_SIZE);

        int64_t now = call->host->clock->current_time();
        if (auto err = io->create(cc->name, cid, s, now, ttl, call->roll))
            return JSI_ThrowTypeError(ctx, "createGranule(%s%s): %s",
                                     cc->name.container.c_str(), key.c_str(), err->c_str());
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

        CONTRACT_DATA_id cid;
        cid.container=key;
        if (!io->exists(cc->name,cid))
            return JSI_ThrowTypeError(ctx, "setGranule: not found: %s%s", cc->name.container.c_str(),cid.container.c_str());

        std::string s = serialize_value(ctx, argv[1]);
        if (!is_serializable(s))
            return JSI_ThrowTypeError(ctx, "setGranule(%s%s): cannot serialize value", cc->name.container.c_str(),cid.container.c_str());
        if (s.size() > MAX_GRANULE_SIZE)
            return JSI_ThrowTypeError(ctx, "setGranule(%s%s): value too large (%zu > %zu)",
                                     cc->name.container.c_str(),cid.container.c_str(), s.size(), MAX_GRANULE_SIZE);

        if (auto err = io->set(cc->name, cid, s, call->roll))
            return JSI_ThrowTypeError(ctx, "setGranule(%s%s): %s",
                                     cc->name.container.c_str(), cid.container.c_str(), err->c_str());
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
        CONTRACT_DATA_id cid;
        cid.container=key;
        std::string value;
        if (!io->get(cc->name,cid, value)) return JSI_UNDEFINED;
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

        JSIScope<20,20> scope(ctx);
        auto name = scope.toStdString(argv[0]);
        if (name.empty()) return JSI_EXCEPTION;
        std::string s = serialize_value(ctx, argv[1]);
        if (!is_serializable(s))
            return JSI_ThrowTypeError(ctx, "emit(%s): cannot serialize value", name.c_str());
        call->emits.push_back(jsiEvent{ cc->name.container, name, s });
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
        CONTRACT_id contract_name;
        contract_name.container = scope.toStdString(argv[0]);
        auto method_name   = scope.toStdString(argv[1]);
        if (contract_name.container.empty() || method_name.empty()) return JSI_EXCEPTION;

        if (self->contract->name == contract_name)
            return JSI_ThrowTypeError(ctx, "callMethod to self not allowed");

        auto cc = self->host->get_contract(contract_name);
        if (!cc.valid())
            return JSI_ThrowTypeError(ctx, "contract not found: %s", contract_name.container.c_str());

        json params = json_from_jsi(ctx, argv[2]);

        jsiMsg empty;
        JSIValue r = cc->call_contract_method(method_name, params, empty, false,
                                          self->call_stack, self->host);

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
        CONTRACT_id contract_name;
        contract_name.container = scope.toStdString(argv[0]);
        auto view_name     = scope.toStdString(argv[1]);
        if (contract_name.container.empty() || view_name.empty()) return JSI_EXCEPTION;

        if (self->contract->name == contract_name)
            return JSI_ThrowTypeError(ctx, "callView to self not allowed");

        auto cc = self->host->get_contract(contract_name);
        if (!cc.valid())
            return JSI_ThrowTypeError(ctx, "contract not found: %s", contract_name.container.c_str());

        json params = json_from_jsi(ctx, argv[2]);

        jsiMsg empty;
        JSIValue r = cc->call_contract_method(view_name, params, empty, true,
                                          self->call_stack, self->host);

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
