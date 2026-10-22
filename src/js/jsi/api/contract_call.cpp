#include "quickjs_i.h"
#include "ContractContext.h"
#include "CallContext.h"
#include "CallFrame.h"
#include "CallStack.h"
#include "ContractHost.h"
#include "contract_constants.h"
#include "convert.h"

using json = nlohmann::json;

JSIValue build_msg_object(JSIContext* ctx, const jsiMsg& m);

JSIValue ContractContext::call_contract_method(const std::string& method_name,
                             const json& params,
                             const jsiMsg& m, bool is_view,
                             const REF_getter<CallStack>& call_stack,
                             const REF_getter<ContractHost>& host)
{
    if (call_stack->call_stack.size() >= (size_t)MAX_CALL_DEPTH)
        return JSI_ThrowTypeError(ctx, "call stack too deep (%zu >= %d)",
                                  call_stack->call_stack.size(), MAX_CALL_DEPTH);

    auto& table = is_view ? views : methods;
    auto it = table.find(method_name);
    if (it == table.end())
        return JSI_ThrowTypeError(ctx, "%s not found: %s",
                                  is_view ? "view" : "method", method_name.c_str());
    JSIValueConst fn = it->second.get();

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

    JSIValue params_js = json_to_jsi(ctx, params);
    if (JSI_IsException(params_js)) {
        JSI_SetContextOpaque(ctx, saved);
        call_stack->call_stack.pop_back();
        return params_js;
    }

    JSIValue msg_obj = build_msg_object(ctx, m);
    JSIValue argv[2] = { params_js, msg_obj };
    int nargs = is_view ? 1 : 2;

    JSIValue r;
    try {
        r = JSI_Call(ctx, fn, JSI_UNDEFINED, nargs, argv);
    } catch (...) {
        JSI_SetContextOpaque(ctx, saved);
        JSI_FreeValue(ctx, msg_obj);
        JSI_FreeValue(ctx, params_js);
        call_stack->call_stack.pop_back();
        throw;
    }

    assert(JSI_GetContextOpaque(ctx) == &call_ctx
           && "opaque was tampered inside JSI_Call");
    JSI_SetContextOpaque(ctx, saved);
    JSI_FreeValue(ctx, msg_obj);
    JSI_FreeValue(ctx, params_js);
    call_stack->call_stack.pop_back();

    if (saved) {
        auto* parent = (CallContext*)saved;
        parent->emits.insert(parent->emits.end(),
                             call_ctx.emits.begin(), call_ctx.emits.end());
    }

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
