#include "quickjs_i.h"
#include "jsi_pack.h"
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

JSIValue build_msg_object(JSIContext* ctx, const jsiMsg& /*m*/) ;

#include "contract_constants.h"

namespace {
    struct ViewInterrupt {
        IMsgSource* gas = nullptr;                  // txView: газ из values
        int64_t ticks_left = MTJS_VIEW_MAX_TICKS;   // без газа (query/reader): локальный лимит
        int64_t per_tick = 1;                       // газ за один tick
    };

    int view_interrupt_handler(JSIRuntime* rt, void* opaque) {
        (void)rt;
        ViewInterrupt* vi = (ViewInterrupt*)opaque;
        if (!vi) return 0;
        if (vi->gas) {
            vi->gas->consume_gas(vi->per_tick);
            if (vi->gas->get_gasUsed() > vi->gas->get_gasLimit()) return 1;   // газ кончился
            return 0;
        }
        if (--vi->ticks_left <= 0) return 1;        // локальный лимит исчерпан
        return 0;
    }
}

// ============================================================
// method: msg есть, газ есть, roll есть
// ============================================================
JSIValue ContractContext::call_method(Cellable* root,const std::string& method_name,
                             const std::string& params_packed,
                             IMsgSource* msg_source,
                             CallStack* call_stack,
                              ContractHost* host, Rollback* roll,
                             const CONTRACT_id& caller, FeesSnapshot* fees,
                             std::set<REF_getter<data_base>> * deleted_granules, TIME_mks block_timestamp)
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
    frame->mode = CallFrame::Mode::Method;
    call_stack->call_stack.push_back(frame);

    REF_getter<TxContext> call_ctx = new TxContext(block_timestamp);
    call_ctx->contract = this;
    call_ctx->msg_source = msg_source;
    call_ctx->call_stack = call_stack;
    call_ctx->host = host;
    call_ctx->roll = roll;
    call_ctx->caller = caller;
    call_ctx->fees = fees;
    call_ctx->deleted_granules = deleted_granules;
    call_ctx->root = root;

    void* saved = JSI_GetContextOpaque(ctx);
    JSI_SetContextOpaque(ctx, call_ctx.get());
    // interrupt handler: прерывание JS по газу (каждые GAS_STEP инструкций)
    JSI_SetInterruptHandler(rt, interrupt_handler, call_ctx.get());
    JSI_UpdateStackTop(rt);

    JSIValue params_js = unpack_jsi(ctx, params_packed);
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
        auto* parent = (TxContext*)saved;
        parent->emits.insert(parent->emits.end(), call_ctx->emits.begin(), call_ctx->emits.end());
    }

    return r;
}

// ============================================================
// view: msg НЕТ, газа НЕТ, roll НЕТ (read-only)
// ============================================================
// ============================================================
// query: reader-only, без газа, now сэмплируется на входе и наследуется вложенными
// ============================================================
JSIValue ContractContext::call_query(Cellable* root, const std::string& query_name,
                             const JSIValue& params,
                             CallStack* call_stack,
                             ContractHost* host,
                             TIME_mks now,
                             TIME_mks block_timestamp)
{
    MUTEX_INSPECTOR;
    if (call_stack->call_stack.size() >= (size_t)MAX_CALL_DEPTH)
        return JSI_ThrowTypeError(ctx, "call stack too deep (%zu >= %d)", call_stack->call_stack.size(), MAX_CALL_DEPTH);

    auto it = queries.find(query_name);
    if (it == queries.end())
        return JSI_ThrowTypeError(ctx, "query not found: %s", query_name.c_str());
    JSIValueConst fn = it->second.get();

    REF_getter<CallFrame> frame = new CallFrame;
    frame->contract = this;
    frame->method = query_name;
    frame->mode = CallFrame::Mode::Query;
    call_stack->call_stack.push_back(frame);

    REF_getter<QueryContext> call_ctx = new QueryContext(now, block_timestamp);
    call_ctx->contract = this;
    call_ctx->msg_source = nullptr;              // в query нет msg
    call_ctx->call_stack = call_stack;
    call_ctx->host = host;
    call_ctx->root = root;

    ViewInterrupt vi;                           // газа нет -> работает tik-бюджет
    vi.gas = nullptr;

    void* saved = JSI_GetContextOpaque(ctx);
    JSI_SetContextOpaque(ctx, call_ctx.get());
    JSI_SetInterruptHandler(rt, view_interrupt_handler, &vi);
    JSI_UpdateStackTop(rt);

    JSIValue params_js = JSI_DupValue(ctx, params);
    if (JSI_IsException(params_js)) {
        JSI_SetInterruptHandler(rt, nullptr, nullptr);
        JSI_SetContextOpaque(ctx, saved);
        call_stack->call_stack.pop_back();
        return params_js;
    }

    JSIValue r = JSI_Call(ctx, fn, JSI_UNDEFINED, 1, &params_js);

    JSI_SetInterruptHandler(rt, nullptr, nullptr);
    JSI_SetContextOpaque(ctx, saved);
    JSI_FreeValue(ctx, params_js);
    call_stack->call_stack.pop_back();
    return r;
}

JSIValue ContractContext::call_view(Cellable* root,const std::string& view_name,
                             const JSIValue& params,
                             IMsgSource* msg_source,
                             CallStack* call_stack,
                             ContractHost* host,
                             int64_t timeout_sec,
                             TIME_mks block_timestamp)
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
    frame->mode = CallFrame::Mode::View;
    call_stack->call_stack.push_back(frame);

    REF_getter<TxViewContext> call_ctx = new TxViewContext(block_timestamp);
    call_ctx->contract = this;
    call_ctx->msg_source = msg_source;       // msg родителя: те же поля, что в методе
    call_ctx->call_stack = call_stack;
    call_ctx->host = host;
    call_ctx->caller = CONTRACT_id{};        // view не читает caller
    call_ctx->root = root;

    ViewInterrupt vi;
    vi.gas = call_ctx->msg_source;                // тот же источник, что у метода: газ из values
    (void)timeout_sec;                           // legacy: лимит теперь по газу

    void* saved = JSI_GetContextOpaque(ctx);
    JSI_SetContextOpaque(ctx, call_ctx.get());
    JSI_SetInterruptHandler(rt, view_interrupt_handler, &vi);   // ← лимит времени
    JSI_UpdateStackTop(rt);

    JSIValue params_js = JSI_DupValue(ctx, params);
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

    // view не эмитит: пробрасывать нечего
    return r;
}