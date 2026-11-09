#include "quickjs_i.h"
#include "ContractContext.h"
#include "CallContext.h"
#include "CallFrame.h"
#include "CallStack.h"
#include "ContractHost.h"
#include "contract_constants.h"
#include "convert.h"
#include "contract_api.h"
#include "t_params.h"
#include "FeesSnapshot.h"

using json = nlohmann::json;
JSIValue build_msg_object(JSIContext* ctx, const jsiMsg& /*m*/) ;

#include <chrono>
#include "contract_constants.h"

namespace {
    struct ViewInterrupt { int64_t deadline_ns; };
    int64_t now_ns() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }
    int view_interrupt_handler(JSIRuntime* rt, void* opaque) {
        (void)rt;
        ViewInterrupt* vi = (ViewInterrupt*)opaque;
        if (!vi) return 0;
        if (now_ns() > vi->deadline_ns) return 1;   // прервать (InternalError: interrupted)
        return 0;
    }
}

// ============================================================
// method: msg есть, газ есть, roll есть
// ============================================================
JSIValue ContractContext::call_contract_method(const std::string& method_name,
                             const json& params,
                             IMsgSource* msg_source,
                             const REF_getter<CallStack>& call_stack,
                             const REF_getter<ContractHost>& host, Rollback* roll,
                             const CONTRACT_id& caller, FeesSnapshot* fees,
                             std::set<REF_getter<data_base>> * deleted_granules)
{
    MUTEX_INSPECTOR;
    if (call_stack->call_stack.size() >= (size_t)MAX_CALL_DEPTH)
        return JSI_ThrowTypeError(ctx, "call stack too deep (%zu >= %d)", call_stack->call_stack.size(), MAX_CALL_DEPTH);

    auto it = methods.find(method_name);
    if (it == methods.end())
        return JSI_ThrowTypeError(ctx, "method not found: %s", method_name.c_str());
    JSIValueConst fn = it->second.get();

    REF_getter<CallFrame> frame = new CallFrame;
    frame->contract = this;
    frame->method = method_name;
    frame->in_view = false;
    call_stack->call_stack.push_back(frame);

    CallContext call_ctx;
    call_ctx.contract = this;
    call_ctx.in_view = false;
    call_ctx.msg_source = msg_source;
    call_ctx.depth = (int)call_stack->call_stack.size() - 1;
    call_ctx.call_stack = call_stack;
    call_ctx.host = host;
    call_ctx.roll = roll;
    call_ctx.caller = caller;
    call_ctx.fees = fees;
    call_ctx.deleted_granules = deleted_granules;
    call_ctx.root = root;

    void* saved = JSI_GetContextOpaque(ctx);
    JSI_SetContextOpaque(ctx, &call_ctx);
    // interrupt handler: прерывание JS по газу (каждые GAS_STEP инструкций)
    JSI_SetInterruptHandler(rt, interrupt_handler, &call_ctx);
    JSI_UpdateStackTop(rt);

    JSIValue params_js = json_to_jsi(ctx, params);
    if (JSI_IsException(params_js)) {
        JSI_SetInterruptHandler(rt, nullptr, nullptr);
        JSI_SetContextOpaque(ctx, saved);
        call_stack->call_stack.pop_back();
        return params_js;
    }

    jsiMsg m;
    JSIValue argv[2];
    argv[0] = params_js;
    argv[1] = build_msg_object(ctx, m);
    int nargs = 2;

    JSIValue r;
    try {
        MUTEX_INSPECTOR;
        r = JSI_Call(ctx, fn, JSI_UNDEFINED, nargs, argv);
    } catch (...) {
        JSI_SetInterruptHandler(rt, nullptr, nullptr);
        JSI_SetContextOpaque(ctx, saved);
        JSI_FreeValue(ctx, argv[1]);
        JSI_FreeValue(ctx, params_js);
        call_stack->call_stack.pop_back();
        throw;
    }

    JSI_SetInterruptHandler(rt, nullptr, nullptr);
    JSI_SetContextOpaque(ctx, saved);
    JSI_FreeValue(ctx, argv[1]);
    JSI_FreeValue(ctx, params_js);
    call_stack->call_stack.pop_back();

    if (saved) {
        MUTEX_INSPECTOR;
        auto* parent = (CallContext*)saved;
        parent->emits.insert(parent->emits.end(), call_ctx.emits.begin(), call_ctx.emits.end());
    }

    return r;
}

// ============================================================
// view: msg НЕТ, газа НЕТ, roll НЕТ (read-only)
// ============================================================
JSIValue ContractContext::call_contract_view(const std::string& view_name,
                             const json& params,
                             const REF_getter<CallStack>& call_stack,
                             const REF_getter<ContractHost>& host,
                             int64_t timeout_sec)
{
    MUTEX_INSPECTOR;
    if (call_stack->call_stack.size() >= (size_t)MAX_CALL_DEPTH)
        return JSI_ThrowTypeError(ctx, "call stack too deep (%zu >= %d)",
                                  call_stack->call_stack.size(), MAX_CALL_DEPTH);

    auto it = views.find(view_name);
    if (it == views.end())
        return JSI_ThrowTypeError(ctx, "view not found: %s", view_name.c_str());
    JSIValueConst fn = it->second.get();

    // клэмп timeout_sec (0..MAX_VIEW_TIMEOUT_SEC)
    if (timeout_sec <= 0) timeout_sec = 5;
    if (timeout_sec > MAX_VIEW_TIMEOUT_SEC) timeout_sec = MAX_VIEW_TIMEOUT_SEC;

    REF_getter<CallFrame> frame = new CallFrame;
    frame->contract = this;
    frame->method = view_name;
    frame->in_view = true;
    call_stack->call_stack.push_back(frame);

    CallContext call_ctx;
    call_ctx.contract = this;
    call_ctx.in_view = true;
    call_ctx.msg_source = nullptr;          // view БЕЗ msg
    call_ctx.depth = (int)call_stack->call_stack.size() - 1;
    call_ctx.call_stack = call_stack;
    call_ctx.host = host;
    call_ctx.roll = nullptr;                // view read-only
    call_ctx.caller = CONTRACT_id{};        // view не читает caller
    call_ctx.fees = nullptr;                // без газа
    call_ctx.deleted_granules = nullptr;
    call_ctx.root = root;

    ViewInterrupt vi;
    vi.deadline_ns = now_ns() + timeout_sec * 1000000000LL;

    void* saved = JSI_GetContextOpaque(ctx);
    JSI_SetContextOpaque(ctx, &call_ctx);
    JSI_SetInterruptHandler(rt, view_interrupt_handler, &vi);   // ← лимит времени
    JSI_UpdateStackTop(rt);

    JSIValue params_js = json_to_jsi(ctx, params);
    if (JSI_IsException(params_js)) {
        JSI_SetInterruptHandler(rt, nullptr, nullptr);
        JSI_SetContextOpaque(ctx, saved);
        call_stack->call_stack.pop_back();
        return params_js;
    }

    // один аргумент: params (msg НЕ передаётся)
    JSIValue r = JSI_Call(ctx, fn, JSI_UNDEFINED, 1, &params_js);

    JSI_SetInterruptHandler(rt, nullptr, nullptr);
    JSI_SetContextOpaque(ctx, saved);
    JSI_FreeValue(ctx, params_js);
    call_stack->call_stack.pop_back();

    if (saved) {
        auto* parent = (CallContext*)saved;
        parent->emits.insert(parent->emits.end(), call_ctx.emits.begin(), call_ctx.emits.end());
    }
    return r;
}