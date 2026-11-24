#include <unistd.h>
#include <string>
#include <version_mega.h>
#include "mtjsService.h"
#include "common/mtjsEvent.h"
#include "event_mt.h"
#include <Events/System/timerEvent.h>
#include <mutexInspector.h>
#include "IUtils.h"
#include <deque>
#include <string>
#include "refstring.h"
#include "version.h"
#include "events_mtjs.hpp"
#include "common/rcf.h"
#include "common/mtjs_opaque.h"
#include <msockaddr_in.h>

#include "common/mtjsEvent.h"
#include "common/jsscope.h"
#include "common/timers.h"
#include "errcodes.h"

#include "sv.h"
#include "md/md_GetUserNonceRSP.h"
#include "md/md_GetContractDataRSP.h"
#include "md/md_GetNodeInfoRSP.h"
#include "md/md_GetNodesRSP.h"
#include "md/md_GetValuesRSP.h"
#include "md/md_GetContractREQ.h"
#include "md/md_GetContractRSP.h"
#include "md/md_GetStateRSP.h"
#include "md/md_GetAddressInfoREQ.h"
#include "md/md_GetAddressInfoRSP.h"
#include "md/md_CallViewREQ.h"
#include "md/md_CallViewRSP.h"
#include "md/md_GetGranuleREQ.h"
#include "md/md_GetGranuleRSP.h"
#include "md/md_ScanREQ.h"
#include "md/md_ScanRSP.h"
// #include <yyjson.h>
#include "js_tools.h"
#include "mtjs_block.h"
extern "C"
{
}

void *thread_worker(void *p)
{
    mtjs_opaque *op = (mtjs_opaque *)p;
    while (!op->async_deque->m_isTerminating && !iUtils->isTerminating())
    {
        auto x = op->async_deque->pop();
        // for(auto x:d)
        if (x.valid())
        {
            x->execute();

            x->listener->listenToEvent(new mtjsEvent::AsyncExecuted(x));
            op->async_deque->decCounter();
        }
        else
        {
            // logErr2("!thread_worker %p", p);
            return NULL;
        }
    }
    return NULL;
}

MTJS::Service::Service(const SERVICE_id &svs, const std::string &nm, IInstance *ifa)
    : UnknownBase(nm),
      ListenerBuffered1Thread(nm, svs, ifa->getConfig()->get_int64_t("STACK_SIZE", 8 * 1024 * 1024, "thread stack size")),
      Broadcaster(ifa), iInstance(ifa)
{
    try
    {
        XTRY;

        thread_cnt = ifa->getConfig()->get_int64_t("THREAD_POOL_SIZE", 10, "js async thread pool count");
        pending_timeout = ifa->getConfig()->get_real("PENDING_TIMEOUT", 0.2, "js pending timeout");
        for (int i = 0; i < thread_cnt; i++)
        {
            size_t stackSize = 8 * 1024 * 2014;
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setstacksize(&attr, stackSize);
            XTRY;
            pthread_t pt;
            if (pthread_create(&pt, &attr, thread_worker, &opaque))
                throw CommonError("pthread_create: errno %d", errno);

            thread_pool.push_back(pt);
            XPASS;
        }

        XPASS;
    }
    catch (const std::exception &e)
    {
        logErr2("exception: %s %s %d", e.what(), __FILE__, __LINE__);
        throw;
    }
}

MTJS::Service::~Service()
{
}

UnknownBase *MTJS::Service::construct(const SERVICE_id &id, const std::string &nm, IInstance *ifa)
{
    XTRY;
    return new Service(id, nm, ifa);
    XPASS;
}

#define CONFIG_MODULE_SUPPORT 1
#include <quickjs.h>
#include <zstd.h>
#include <lz4.h>
#include "ioBuffer.h"
#include "i128.h"
// mallinfo2 is glibc-specific and malloc.h is not available on macOS.
#if defined(__GLIBC__)
#include <malloc.h>
#endif
#include <stdio.h>

void print_mem_stats() {
#if defined(__GLIBC__)
    struct mallinfo2 mi = mallinfo2();

    printf("=== Memory Stats ===\n");
    printf("Занято приложением:  %zu MB\n", mi.uordblks / 1024 / 1024);
    printf("Свободно в куче:     %zu MB\n", mi.fordblks / 1024 / 1024);
    printf("Куча от ОС (brk):    %zu MB\n", mi.arena / 1024 / 1024);
    printf("Память через mmap:   %zu MB\n", mi.hblkhd / 1024 / 1024);
    printf("Можно вернуть ОС:    %zu MB\n", mi.keepcost / 1024 / 1024);
    printf("====================\n");
#else
    printf("=== Memory Stats ===\n");
    printf("Detailed malloc statistics are unavailable on this platform\n");
    printf("====================\n");
#endif
}
void dump_memory_usage(JSRuntime *runtime)
{
    JSMemoryUsage mem_usage;
    JS_ComputeMemoryUsage(runtime, &mem_usage);      // Заполняем структуру статистики
    JS_DumpMemoryUsage(stdout, &mem_usage, runtime); // Выводим в консоль

    printf("\n");
    print_mem_stats();
}

void MTJS::Service::load_config()
{
}

void registerMTJSModule(const char *pn)
{
    if (pn)
        iUtils->registerPlugingInfo(pn, IUtils::PLUGIN_TYPE_SERVICE, ServiceEnum::mtjs, "MTJS", getEvents_mtjs());
    else
    {
        iUtils->registerService(ServiceEnum::mtjs, MTJS::Service::construct, "MTJS");
        regEvents_mtjs();
    }
}

bool MTJS::Service::handleEvent(const REF_getter<Event::Base> &e)
{
    XTRY;
    auto &ID = e->id;

    switch (ID)
    {
    case systemEventEnum::startService:
        return on_startService((const systemEvent::startService *)e.get());
    case timerEventEnum::TickTimer:
        return TickTimer((const timerEvent::TickTimer *)e.get());
    case httpEventEnum::RequestIncoming:
        return RequestIncoming((const httpEvent::RequestIncoming *)e.get());
    case httpEventEnum::RequestStartChunking:
        return RequestStartChunking((const httpEvent::RequestStartChunking *)e.get());
    case httpEventEnum::RequestChunkReceived:
        return RequestChunkReceived((const httpEvent::RequestChunkReceived *)e.get());
    case httpEventEnum::RequestChunkingCompleted:
        return RequestChunkingCompleted((const httpEvent::RequestChunkingCompleted *)e.get());
    case httpEventEnum::WSDisaccepted:
        return WSDisaccepted((const httpEvent::WSDisaccepted *)e.get());
    case httpEventEnum::WSDisconnected:
        return WSDisconnected((const httpEvent::WSDisconnected *)e.get());
    case httpEventEnum::WSTextMessage:
        return WSTextMessage((const httpEvent::WSTextMessage *)e.get());
    case socketEventEnum::Connected:
        return Connected((const socketEvent::Connected *)e.get());
    case socketEventEnum::Disconnected:
        return Disconnected((const socketEvent::Disconnected *)e.get());
    case socketEventEnum::StreamRead:
        return StreamRead((const socketEvent::StreamRead *)e.get());
    case socketEventEnum::NotifyOutBufferEmpty:
        return NotifyOutBufferEmpty((const socketEvent::NotifyOutBufferEmpty *)e.get());
    case timerEventEnum::TickAlarm:
        return TickAlarm((const timerEvent::TickAlarm *)e.get());
    case mtjsEventEnum::AsyncExecuted:
        return AsyncExecuted((const mtjsEvent::AsyncExecuted *)e.get());

    case mtjsEventEnum::Eval:
        return Eval((const mtjsEvent::Eval *)e.get());
    case mtjsEventEnum::EmitterData:
        return EmitterData((const mtjsEvent::EmitterData *)e.get());
    case telnetEventEnum::CommandEntered:
        return CommandEntered((const telnetEvent::CommandEntered *)e.get());
    case bcEventEnum::ClientMsgReply:
        return ClientMsgReply((const bcEvent::ClientMsgReply *)e.get());

    case bcEventEnum::ClientTxSubscribeRSP:
        return ClientTxSubscribeRSP((const bcEvent::ClientTxSubscribeRSP *)e.get());

    case bcEventEnum::AddTxRSP:
        return AddTxRSP((const bcEvent::AddTxRSP *)e.get());

#ifdef WEBDUMP
    case webHandlerEventEnum::RequestIncoming:
        return RequestIncoming((const webHandlerEvent::RequestIncoming *)e.get());
#endif

    case rpcEventEnum::IncomingOnConnector:
    {
        XTRY;
        const rpcEvent::IncomingOnConnector *E = (const rpcEvent::IncomingOnConnector *)e.get();
        auto &IDC = E->e->id;
        switch (IDC)
        {
        case bcEventEnum::AddTxRSP:
            return AddTxRSP((const bcEvent::AddTxRSP *)E->e.get());
        case mtjsEventEnum::mtjsRpcREQ:
            return mtjsRpcREQ((mtjsEvent::mtjsRpcREQ *)E->e.get(), E->esi);
        case mtjsEventEnum::mtjsRpcRSP:
            return mtjsRpcRSP((mtjsEvent::mtjsRpcRSP *)E->e.get(), E->esi);
        case bcEventEnum::ClientMsgReply:
            return ClientMsgReply((const bcEvent::ClientMsgReply *)E->e.get());
        case bcEventEnum::ClientTxSubscribeRSP:
            return ClientTxSubscribeRSP((const bcEvent::ClientTxSubscribeRSP *)E->e.get());

        default:
            logErr2("unhandled event %s %s %d", iUtils->genum_name(E->e->id), __func__, __LINE__);
            break;
        }
        logErr2("unhandled IncomingOnConnector event %s %s %d", iUtils->genum_name(E->e->id), __func__, __LINE__);
        XPASS;
    }
    case rpcEventEnum::IncomingOnAcceptor:
    {
        XTRY;
        const rpcEvent::IncomingOnAcceptor *E = (const rpcEvent::IncomingOnAcceptor *)e.get();
        auto &IDA = E->e->id;
        switch (IDA)
        {
        case bcEventEnum::AddTxRSP:
            return AddTxRSP((const bcEvent::AddTxRSP *)E->e.get());
        case mtjsEventEnum::mtjsRpcREQ:
            return mtjsRpcREQ((mtjsEvent::mtjsRpcREQ *)E->e.get(), E->esi);
        case mtjsEventEnum::mtjsRpcRSP:
            return mtjsRpcRSP((mtjsEvent::mtjsRpcRSP *)E->e.get(), E->esi);
        case bcEventEnum::ClientMsgReply:
            return ClientMsgReply((const bcEvent::ClientMsgReply *)E->e.get());
        case bcEventEnum::ClientTxSubscribeRSP:
            return ClientTxSubscribeRSP((const bcEvent::ClientTxSubscribeRSP *)E->e.get());
        default:
            logErr2("unhandled event %s %s %d", iUtils->genum_name(E->e->id), __func__, __LINE__);
            break;
        }
        logErr2("unhandled IncomingOnAcceptor event %s %s %d", iUtils->genum_name(E->e->id), __func__, __LINE__);
        XPASS;
    }

    default:
        logErr2("unhandled event %s", iUtils->genum_name(e->id));
        return false;
    }
    XPASS;
    return false;
}

void MTJS::Service::checkForExit()
{
    executePending();
    if (getPendingCount())
        return;
    if (opaque.tx_wait_promises.size()) return;
    if (opaque.rpcBlockExit)
        return;
    if(opaque.node_req_promises.size())
        return;
    if(opaque.tx_subscription_cb.has_value())
        return;

    {
        M_LOCKC(opaque.async_deque->m_mutex);
        if (opaque.async_deque->container.size())
            return;
    }
    if (opaque.rcf->servers.size())
        return;
    if (opaque.rcf->timers.timers_refed.size())
        return;
    if (opaque.rcf->block_services.size())
        return;
    if (!opaque.async_deque->empty())
        return;
    stop();
    exit(0);
}

JSValue makeError(JSContext *ctx, int errid, const char *errstr)
{
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "errcode", JS_NewInt32(ctx, errid));
    JS_SetPropertyStr(ctx, obj, "errstr", JS_NewString(ctx, errstr));
    return obj;
}
bool MTJS::Service::TickAlarm(const timerEvent::TickAlarm *e)
{
    MUTEX_INSPECTOR;
    if (e->tid == Timers::TIMER_ClientMsg_TIMEDOUT)
    {
        // logErr2("if(e->tid==Timers::TIMER_ClientMsg_TIMEDOUT)");
        JSScope<10, 10> scope(js_ctx);
        std::string h;
        h = e->data.get()->container;
        auto it = opaque.node_req_promises.find(h);
        if (it == opaque.node_req_promises.end())
        {
            DBG(logErr2("TickAlarm: TIMER_ClientMsg_TIMEDOUT for %s — already resolved (skip)",
                base16::encode(h).substr(0,8).c_str()));
            return true; 
        }
        // it->second.ctx

        JSValue global_obj = JS_GetGlobalObject(js_ctx);
        auto o = makeError(it->second.ctx, BCERR_TIMEDOUT, "timed out");
        scope.addValue(o);
        // logErr2("")
        JSValue func_result = JS_Call(it->second.ctx, it->second.reject.get(), global_obj, 1, &o);
        scope.addValue(func_result);
        opaque.node_req_promises.erase(h);
        return true;
        // JS_Call
    }
    if (e->tid == Timers::TIMER_TX_WAIT_BLOCK)
    {
        JSScope<10, 10> scope(js_ctx);
        std::string h = e->data->container;
        // logErr2("[mtjs] TickAlarm TX_WAIT_BLOCK h=%s node_tx_cb=%d tx_wait_promises=%d",
        //     base16::encode(h).c_str(), (int)opaque.node_tx_cb.size(), (int)opaque.tx_wait_promises.size());

        // 1) tx_wait_promises (hex) — tx_wait_block, если есть
        auto it = opaque.tx_wait_promises.find(h);
        if (it != opaque.tx_wait_promises.end()) {
            JSValue global_obj = JS_GetGlobalObject(js_ctx);
            scope.addValue(global_obj);
            JSValue o = makeError(it->second.ctx, BCERR_TIMEDOUT, "timed out");
            scope.addValue(o);
            JSValue r = JS_Call(it->second.ctx, it->second.reject.get(), global_obj, 1, &o);
            scope.addValue(r);
            opaque.tx_wait_promises.erase(it);
        }

        // 2) node_tx_cb (бинарный) — tx_submit(cb) → timeout, независимо
        auto cbIt = opaque.node_tx_cb.find(h);
        if (cbIt != opaque.node_tx_cb.end()) {
            // logErr2("[mtjs] TickAlarm cb BEFORE h=%s", base16::encode(h).c_str());
            JSValue obj = JS_NewObject(js_ctx);
            scope.addValue(obj);
            JS_SetPropertyStr(js_ctx, obj, "tx_hash", JS_NewString(js_ctx, base16::encode(h).c_str()));  // hex!
            JS_SetPropertyStr(js_ctx, obj, "status", JS_NewString(js_ctx, "timeout"));
            JSValue ret = JS_Call(js_ctx, cbIt->second.get(), JS_UNDEFINED, 1, &obj);
            scope.addValue(ret);
            // logErr2("[mtjs] TickAlarm cb AFTER exc=%d", JS_IsException(ret) ? 1 : 0);
            qjs::checkForException(js_ctx, ret, "TickAlarm: node_tx_cb timeout");
            opaque.node_tx_cb.erase(cbIt);
        }

        return true;
    }
    if (e->tid == Timers::TIMER_TIMER)
    {
        JSScope<10, 10> scope(js_ctx);
        TimerTask *t = (TimerTask *)e->cookie.get();
        JSValue global_obj = JS_GetGlobalObject(js_ctx);
        scope.addValue(global_obj);

        JSValue ac[t->fargs.size()];
        for (int i = 0; i < t->fargs.size(); i++)
        {
            ac[i] = t->fargs[i].get();
        }
        JSValue func_result = JS_Call(js_ctx, t->cb.get(), global_obj, t->fargs.size(), ac);
        scope.addValue(func_result);
        auto id = std::stoll(e->data->container);
        // logErr2("TickAlarm: %lld", id);
        rconf->timers.timers_refed.erase(id);
        rconf->timers.timers_unrefed.erase(id);

        return true;
    }
    else
        throw CommonError("if(e->tid==RCF::TIMER_TIMER)");
    return false;
}
void MTJS::Service::executePending()
{
    for (;;)
    {
        int r = JS_ExecutePendingJob(js_rt, NULL);
        if (r < 0)
        {
            fprintf(stderr, "[Loop] JS_ExecutePendingJob => error.\n");
            return;
        }
        if (r == 0)
            break;
    }
}
bool MTJS::Service::TickTimer(const timerEvent::TickTimer *e)
{
    MUTEX_INSPECTOR;
    JSScope<10, 10> scope(js_ctx);
    if (e->tid == Timers::TIMER_REPORT_MEM)
    {
        // printf("REPORT_MEM MTJS ");
        // dump_memory_usage(js_rt);
        // printf("\n");
        return true;
    }
    else if (e->tid == Timers::TIMER_INTERVAL)
    {
        MUTEX_INSPECTOR;
        TimerTask *t = (TimerTask *)e->cookie.get();
        // logErr2("TickTimer: %lld", std::stoll(e->data->container));
        JSValue global_obj = JS_GetGlobalObject(js_ctx);
        scope.addValue(global_obj);
        JSValue ac[t->fargs.size()];
        for (int i = 0; i < t->fargs.size(); i++)
        {
            ac[i] = t->fargs[i].get();
        }
        JSValue func_result = JS_Call(js_ctx, t->cb.get(), global_obj, t->fargs.size(), ac);

        scope.addValue(func_result);
        qjs::checkForException(js_ctx, func_result, "TickTimer: JS_Call");
        return true;
    }
    else if (e->tid == Timers::TIMER_POLL)
    {
        MUTEX_INSPECTOR;
        executePending();
        checkForExit();

        return true;
    }
    return false;
}

JSValue loadModule(JSContext *ctx, const std::string &moduleCode)
{
    JSScope<10, 10> scope(ctx);
    JSValue module1 = JS_Eval(ctx, moduleCode.c_str(), moduleCode.size(), "<module>", JS_EVAL_TYPE_MODULE | JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_COMPILE_ONLY);

    if (qjs::checkAndPrintException(ctx, module1, "loadModule"))
        return module1;

    JSValue ret = JS_EvalFunction(ctx, module1);
    scope.addValue(ret);
    if (qjs::checkAndPrintException(ctx, ret, "loadModule"))
        return ret;

    if (JS_IsException(ret))
    {
        JSValue exc_obj = JS_GetException(ctx);
        scope.addValue(exc_obj);
        auto message(scope.toStdStringView(exc_obj));
        JSValue stack_val = JS_GetPropertyStr(ctx, exc_obj, "stack");
        auto stack = JS_IsUndefined(stack_val) ? "No stack available" : scope.toStdStringView(stack_val);

        if (stack.size())
        {
            fprintf(stderr, "Stack trace:\n" SV_FMT "\n", SV_ARG(stack));
        }
        if (message.size())
        {
            fprintf(stderr, "[loadModule] Runtime error: " SV_FMT "\n", SV_ARG(message));
        }
    }
    // #endif
    return module1;
}

bool MTJS::Service::Eval(const mtjsEvent::Eval *e)
{
    module__ = loadModule(js_ctx, e->js);

    executePending();
    checkForExit();

    return true;
}

#ifdef WEBDUMP
bool MTJS::Service::RequestIncoming(const webHandlerEvent::RequestIncoming *e)
{
    auto d = iUtils->mem_dump();
    std::string out;

    for (auto z : d)
    {
        out += z.first + " " + std::to_string(z.second) + "\n";
    }
    HTTP::Response resp(e->req);
    resp.make_response(out);
    return true;
}
#endif
JSValue create_uint8array(JSContext *ctx, const char *data, size_t len)
{
    MUTEX_INSPECTOR;
    JSValue array_buffer = JS_NewArrayBufferCopy(ctx, (const uint8_t *)data, len);
    if (JS_IsException(array_buffer))
    {
        return JS_EXCEPTION;
    }
    return array_buffer;
}
bool MTJS::Service::EmitterData(const mtjsEvent::EmitterData *e)
{
    MUTEX_INSPECTOR;

    JSScope<10, 10> scope(js_ctx);
    auto sValue = create_uint8array(js_ctx, e->data.data(), e->data.size());
    scope.addValue(sValue);

    e->emitter->emit(e->cmd, 1, &sValue);
    return true;
}
JSValue js_telnet_request_new(JSContext *ctx, const REF_getter<telnetEvent::CommandEntered> &request);

bool MTJS::Service::CommandEntered(const telnetEvent::CommandEntered *e)
{
    MUTEX_INSPECTOR;
    if (opaque.telnet_callback.has_value())
    {
        JSScope<10, 10> scope(js_ctx);
        JSValueGuard telnetCallback = *opaque.telnet_callback;
        if (JS_IsFunction(js_ctx, telnetCallback.get()))
        {

            JSValue args = js_telnet_request_new(js_ctx, e);
            scope.addValue(args);
            JSValue result = JS_Call(js_ctx, telnetCallback.get(), JS_UNDEFINED, 1, &args);
            scope.addValue(result);
            qjs::checkForException(js_ctx, result, "CommandEntered: JS_Call");
        }
    }
    return true;
}

#include "msg.h"
// #include "quickjspp.hpp"
bool MTJS::Service::AddTxRSP(const bcEvent::AddTxRSP* e)
{



    auto it = opaque.node_req_promises.find(e->tx_hash.container);
    if (it == opaque.node_req_promises.end())
        throw CommonError("if(it==opaque.node_req_promises.end())");


    // msg::transaction_added_rsp r;
    // r.unpack(in);
    auto *ctx = it->second.ctx;
    JSScope<20, 20> scope(it->second.ctx);
    auto obj = JS_NewObject(ctx);
    scope.addValue(obj);
    JS_SetPropertyStr(ctx, obj, "err", JS_NewInt32(ctx, e->errcode));
    JS_SetPropertyStr(ctx, obj, "err_str", JS_NewString(ctx, e->errmsg.c_str()));
    JS_SetPropertyStr(ctx, obj, "tx_hash", JS_NewString(ctx, base16::encode(e->tx_hash.container).c_str()));

    JSValue ret = JS_Call(it->second.ctx, it->second.resolve.get(), JS_UNDEFINED, 1, &obj);

    scope.addValue(ret);

    opaque.node_req_promises.erase(e->tx_hash.container);

    {
        auto cbIt = opaque.node_tx_cb.find(e->tx_hash.container);
        if (cbIt != opaque.node_tx_cb.end()) {
            JSValue obj = JS_NewObject(ctx);
            scope.addValue(obj);
            JS_SetPropertyStr(ctx, obj, "tx_hash", JS_NewString(ctx, base16::encode(e->tx_hash.container).c_str()));
            JS_SetPropertyStr(ctx, obj, "status", JS_NewString(ctx, e->errcode == 0 ? "submitted" : "err"));
            if (e->errcode != 0) JS_SetPropertyStr(ctx, obj, "errstr", JS_NewString(ctx, e->errmsg.c_str()));
            JSValue ret = JS_Call(js_ctx, cbIt->second.get(), JS_UNDEFINED, 1, &obj);
            scope.addValue(ret);
            qjs::checkForException(js_ctx, ret, "AddTxRSP: node_tx_cb");
            if (e->errcode != 0) {
                opaque.node_tx_cb.erase(cbIt);
                sendEvent(ServiceEnum::Timer, new timerEvent::StopAlarm(Timers::TIMER_TX_WAIT_BLOCK, toRef(e->tx_hash.container), this));
            }
        }
    }
    sendEvent(ServiceEnum::Timer, new timerEvent::StopAlarm(Timers::TIMER_ClientMsg_TIMEDOUT, toRef(e->tx_hash.container), this));

    return true;

}
// ============================================================
// Распаковка гранулы (формат jsi_pack: [format_byte][payload])
// нативное JS_* (mtjs-клиент) — зеркало jsi_pack (qjsi-контракты).
// ============================================================
static JSValue mtjs_i128_to_bigint(JSContext* ctx, __int128 v) {
    std::string str = i128(v).str();
    JSValue g = JS_GetGlobalObject(ctx);
    JSValue fn = JS_GetPropertyStr(ctx, g, "BigInt");
    if (JS_IsException(fn) || JS_IsUndefined(fn)) {
        JS_FreeValue(ctx, fn); JS_FreeValue(ctx, g);
        return JS_NULL;
    }
    JSValue arg = JS_NewStringLen(ctx, str.data(), str.size());
    JSValue out = JS_Call(ctx, fn, g, 1, &arg);
    JS_FreeValue(ctx, arg); JS_FreeValue(ctx, fn); JS_FreeValue(ctx, g);
    return out;
}

enum class MtjsTag : int64_t {
    Null = 0, Bool = 1, Int = 2, Str = 3, Array = 4, Object = 5,
};

static JSValue mtjs_unpack_impl(JSContext* ctx, inBuffer& in) {
    int64_t tag = 0;
    in >> tag;
    switch (tag) {
        case (int64_t)MtjsTag::Null:   return JS_NULL;
        case (int64_t)MtjsTag::Bool: { bool b = false; in >> b; return JS_NewBool(ctx, b); }
        case (int64_t)MtjsTag::Int: { __int128 v = in.get_i128(); return mtjs_i128_to_bigint(ctx, v); }
        case (int64_t)MtjsTag::Str: { std::string s; in >> s; return JS_NewStringLen(ctx, s.data(), s.size()); }
        case (int64_t)MtjsTag::Array: {
            int64_t n = 0; in >> n;
            JSValue arr = JS_NewArray(ctx);
            for (int64_t i = 0; i < n; ++i) {
                JSValue v = mtjs_unpack_impl(ctx, in);
                JS_SetPropertyUint32(ctx, arr, (uint32_t)i, v);
            }
            return arr;
        }
        case (int64_t)MtjsTag::Object: {
            int64_t n = 0; in >> n;
            JSValue obj = JS_NewObject(ctx);
            for (int64_t i = 0; i < n; ++i) {
                std::string k; in >> k;
                JSValue v = mtjs_unpack_impl(ctx, in);
                JS_SetPropertyStr(ctx, obj, k.c_str(), v);
            }
            return obj;
        }
    }
    return JS_NULL;
}

JSValue mtjs_granule_unpack(JSContext* ctx, const std::string& container) {
    if (container.empty()) return JS_NULL;
    uint8_t fmt = (uint8_t)container[0];
    std::string payload = container.substr(1);
    if (fmt == 1) {
        unsigned long long sz = ZSTD_getFrameContentSize(payload.data(), payload.size());
        if (sz == ZSTD_CONTENTSIZE_ERROR || sz == ZSTD_CONTENTSIZE_UNKNOWN) return JS_NULL;
        std::string raw((size_t)sz, '\0');
        size_t r = ZSTD_decompress(&raw[0], raw.size(), payload.data(), payload.size());
        if (ZSTD_isError(r)) return JS_NULL;
        raw.resize(r);
        payload.swap(raw);
    } else if (fmt == 2) {
        // [orig_size(4)][lz4_data]
        if (payload.size() < 4) return JS_NULL;
        uint32_t orig = 0; memcpy(&orig, payload.data(), 4);
        std::string dst(orig, '\0');
        int n = LZ4_decompress_safe(payload.data() + 4, &dst[0], (int)(payload.size() - 4), (int)orig);
        if (n < 0) return JS_NULL;
        dst.resize(n);
        payload.swap(dst);
    } else if (fmt != 0) {
        return JS_NULL;
    }
    inBuffer in(payload);
    return mtjs_unpack_impl(ctx, in);
}

JSValue mtjs_unpack_bytes(JSContext* ctx, const std::string& bytes) {
    inBuffer in(bytes);
    return mtjs_unpack_impl(ctx, in);
}

// Нативная упаковка JSValue -> [оформление jsi_pack] (зеркало jsi_pack, клиент).
// Формат совпадает с nodой: pack_jsi (тег + i128 + string/array/object).
static void mtjs_pack_impl(JSContext* ctx, JSValueConst val, outBuffer& o) {
    if (JS_IsNull(val) || JS_IsUndefined(val)) { o << (int64_t)MtjsTag::Null; return; }
    if (JS_IsBool(val)) { o << (int64_t)MtjsTag::Bool; o << (bool)(JS_ToBool(ctx, val) != 0); return; }
    int tag = JS_VALUE_GET_TAG(val);
    if (tag == JS_TAG_BIG_INT || tag == JS_TAG_SHORT_BIG_INT) {
        size_t len = 0;
        const char* s = JS_ToCStringLen(ctx, &len, val);
        if (!s) throw std::runtime_error("pack: bigint");
        std::string str(s, len);
        JS_FreeCString(ctx, s);
        o << (int64_t)MtjsTag::Int;
        o.put_i128(i128::fromString(str).v);
        return;
    }
    if (tag == JS_TAG_INT) { o << (int64_t)MtjsTag::Int; o.put_i128((__int128)JS_VALUE_GET_INT(val)); return; }
    if (JS_IsString(val)) {
        size_t len = 0;
        const char* s = JS_ToCStringLen(ctx, &len, val);
        std::string r(s ? s : "", s ? len : 0);
        if (s) JS_FreeCString(ctx, s);
        o << (int64_t)MtjsTag::Str; o << r; return;
    }
    if (JS_IsArray(ctx, val)) {
        JSValue lv = JS_GetPropertyStr(ctx, val, "length");
        int64_t len = 0; JS_ToInt64(ctx, &len, lv); JS_FreeValue(ctx, lv);
        o << (int64_t)MtjsTag::Array; o << (int64_t)len;
        for (int64_t i = 0; i < len; ++i) {
            JSValue item = JS_GetPropertyUint32(ctx, val, (uint32_t)i);
            mtjs_pack_impl(ctx, item, o);
            JS_FreeValue(ctx, item);
        }
        return;
    }
    if (JS_IsObject(val)) {
        JSPropertyEnum* props = nullptr;
        uint32_t count = 0;
        std::map<std::string, JSValue> items;
        if (JS_GetOwnPropertyNames(ctx, &props, &count, val, JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) == 0) {
            for (uint32_t i = 0; i < count; ++i) {
                const char* k = JS_AtomToCString(ctx, props[i].atom);
                std::string key = k ? k : "";
                if (k) JS_FreeCString(ctx, k);
                JSValue item = JS_GetProperty(ctx, val, props[i].atom);
                items[key] = item;
                JS_FreeAtom(ctx, props[i].atom);
            }
            js_free(ctx, props);
        }
        o << (int64_t)MtjsTag::Object; o << (int64_t)items.size();
        for (auto& kv : items) {
            o << kv.first;
            mtjs_pack_impl(ctx, kv.second, o);
            JS_FreeValue(ctx, kv.second);
        }
        return;
    }
    throw std::runtime_error("pack: unsupported value");
}

JSValue mtjs_pack_bytes(JSContext* ctx, JSValueConst val, std::string& out) {
    outBuffer o;
    mtjs_pack_impl(ctx, val, o);
    out = o.asString()->container;
    return JS_UNDEFINED;
}

bool MTJS::Service::ClientMsgReply(const bcEvent::ClientMsgReply *e)
{
    MUTEX_INSPECTOR;
    // logErr2("ClientMsgReply");
    auto it = opaque.node_req_promises.find(e->hash_of_request.container);
    if (it == opaque.node_req_promises.end())
        throw CommonError("if(it==opaque.node_req_promises.end())");

    inBuffer in(e->msg);
    auto p = in.get_PN();
    REF_getter<MsgData::Base> b=msgFactory.create(p);
    b->unpack(in);
    switch(p)
    {

// в ClientMsgReply switch(p):
    case msgid::ScanRSP:
    {
        auto *rs = (MsgData::ScanRSP*)b.get();
        auto *ctx = it->second.ctx;
        JSScope<20,20> scope(ctx);
        auto obj = JS_NewObject(ctx);
        scope.addValue(obj);
        JSValue dbids_arr = JS_NewArray(ctx);
        uint32_t i = 0;
        for (auto& d : rs->dbids) {
            JS_SetPropertyUint32(ctx, dbids_arr, i++, JS_NewString(ctx, base16::encode(d.container).c_str()));
        }
        JS_SetPropertyStr(ctx, obj, "dbids", dbids_arr);
        JS_SetPropertyStr(ctx, obj, "next_from", JS_NewString(ctx, rs->next_from.container.empty() ? "" : base16::encode(rs->next_from.container).c_str()));
        JS_SetPropertyStr(ctx, obj, "done", JS_NewBool(ctx, rs->done));
        JS_SetPropertyStr(ctx, obj, "scanned", JS_NewInt64(ctx, rs->scanned));
        JSValue ret = JS_Call(ctx, it->second.resolve.get(), JS_UNDEFINED, 1, &obj);
        scope.addValue(ret);
        opaque.node_req_promises.erase(e->hash_of_request.container);
        sendEvent(ServiceEnum::Timer, new timerEvent::StopAlarm(Timers::TIMER_ClientMsg_TIMEDOUT, toRef(e->hash_of_request.container), this));
        return true;
    }

    case msgid::GetGranuleRSP:
    {
        auto *rs = (MsgData::GetGranuleRSP*)b.get();
        auto *ctx = it->second.ctx;
        JSScope<20,20> scope(ctx);
        auto obj = JS_NewObject(ctx);
        scope.addValue(obj);
        JS_SetPropertyStr(ctx, obj, "found", JS_NewBool(ctx, rs->found()));
        JS_SetPropertyStr(ctx, obj, "errcode", JS_NewInt32(ctx, rs->errcode));
        JS_SetPropertyStr(ctx, obj, "errstr",  JS_NewString(ctx, rs->errstr.c_str()));
        JS_SetPropertyStr(ctx, obj, "stale", JS_NewBool(ctx, rs->stale));
        JS_SetPropertyStr(ctx, obj, "body", JS_NewString(ctx, base16::encode(rs->body).c_str()));
        JSValue ret = JS_Call(ctx, it->second.resolve.get(), JS_UNDEFINED, 1, &obj);
        scope.addValue(ret);
        opaque.node_req_promises.erase(e->hash_of_request.container);
        sendEvent(ServiceEnum::Timer, new timerEvent::StopAlarm(Timers::TIMER_ClientMsg_TIMEDOUT, toRef(e->hash_of_request.container), this));
        return true;
    }
    case msgid::CallViewRSP:
    {
        auto *rs = (MsgData::CallViewRSP*)b.get();
        auto *ctx = it->second.ctx;
        JSScope<20,20> scope(ctx);
        auto obj = JS_NewObject(ctx);
        scope.addValue(obj);
        JS_SetPropertyStr(ctx, obj, "found", JS_NewBool(ctx, rs->found()));
        JS_SetPropertyStr(ctx, obj, "errcode", JS_NewInt32(ctx, rs->errcode));
        JS_SetPropertyStr(ctx, obj, "errstr",  JS_NewString(ctx, rs->errstr.c_str()));        
        JS_SetPropertyStr(ctx, obj, "error", JS_NewBool(ctx, rs->error));
        JS_SetPropertyStr(ctx, obj, "result", rs->error
            ? JS_NewString(ctx, rs->result_json.c_str())
            : mtjs_granule_unpack(ctx, rs->result_json));
        JSValue ret = JS_Call(ctx, it->second.resolve.get(), JS_UNDEFINED, 1, &obj);
        scope.addValue(ret);
        opaque.node_req_promises.erase(e->hash_of_request.container);
        sendEvent(ServiceEnum::Timer, new timerEvent::StopAlarm(Timers::TIMER_ClientMsg_TIMEDOUT, toRef(e->hash_of_request.container), this));
        return true;
    }
    case msgid::GetAddressInfoRSP: {
        auto *rs = (MsgData::GetAddressInfoRSP*)b.get();
        auto *ctx = it->second.ctx;
        JSScope<20,20> scope(ctx);
        auto obj = JS_NewObject(ctx);
        scope.addValue(obj);
        JS_SetPropertyStr(ctx, obj, "found", JS_NewBool(ctx, rs->found()));
        JS_SetPropertyStr(ctx, obj, "balance", JS_NewString(ctx, std::to_string(rs->balance).c_str()));
        JS_SetPropertyStr(ctx, obj, "nonce", JS_NewString(ctx, std::to_string(rs->nonce).c_str()));
        JSValue keys_arr = JS_NewArray(ctx);
        uint32_t ki = 0;
        for (auto& k : rs->keys) {
            JSValue ko = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, ko, "id", JS_NewInt64(ctx, k.id));
            JS_SetPropertyStr(ctx, ko, "type", JS_NewInt32(ctx, k.type));
            JS_SetPropertyStr(ctx, ko, "keybuffer", JS_NewString(ctx, k.keybuffer.c_str()));
            JS_SetPropertyUint32(ctx, keys_arr, ki++, ko);
        }
        JS_SetPropertyStr(ctx, obj, "keys", keys_arr);
        JSValue ret = JS_Call(ctx, it->second.resolve.get(), JS_UNDEFINED, 1, &obj);
        scope.addValue(ret);
        opaque.node_req_promises.erase(e->hash_of_request.container);
        sendEvent(ServiceEnum::Timer, new timerEvent::StopAlarm(Timers::TIMER_ClientMsg_TIMEDOUT, toRef(e->hash_of_request.container), this));
        return true;
    }
    case msgid::GetStateRSP:
    {
        auto *rs = (MsgData::GetStateRSP*) b.get();
        auto *ctx = it->second.ctx;
        JSScope<20, 20> scope(it->second.ctx);
        auto obj = JS_NewObject(ctx);
        scope.addValue(obj);
        JS_SetPropertyStr(ctx, obj, "found", JS_NewBool(ctx, rs->found()));
        JS_SetPropertyStr(ctx, obj, "errcode", JS_NewInt32(ctx, rs->errcode));
        JS_SetPropertyStr(ctx, obj, "errstr",  JS_NewString(ctx, rs->errstr.c_str()));        
        JS_SetPropertyStr(ctx, obj, "total_supply", JS_NewString(ctx, rs->total_supply.c_str()));
        JS_SetPropertyStr(ctx, obj, "total_stake",  JS_NewString(ctx, rs->total_stake.c_str()));
        JS_SetPropertyStr(ctx, obj, "account_count",  JS_NewInt64(ctx, rs->account_count));
        JS_SetPropertyStr(ctx, obj, "node_count",     JS_NewInt64(ctx, rs->node_count));
        JS_SetPropertyStr(ctx, obj, "contract_count", JS_NewInt64(ctx, rs->contract_count));
        JS_SetPropertyStr(ctx, obj, "tx_count",       JS_NewInt64(ctx, rs->tx_count));
        JS_SetPropertyStr(ctx, obj, "block_height",   JS_NewInt64(ctx, rs->block_height));
        JS_SetPropertyStr(ctx, obj, "genesis_time",   JS_NewInt64(ctx, rs->genesis_time));
        JS_SetPropertyStr(ctx, obj, "root_hash", JS_NewString(ctx, rs->root_hash.c_str()));
        JS_SetPropertyStr(ctx, obj, "chain_id",       JS_NewInt64(ctx, rs->chain_id));
        JSValue ret = JS_Call(it->second.ctx, it->second.resolve.get(), JS_UNDEFINED, 1, &obj);
        scope.addValue(ret);
        opaque.node_req_promises.erase(e->hash_of_request.container);
        sendEvent(ServiceEnum::Timer, new timerEvent::StopAlarm(Timers::TIMER_ClientMsg_TIMEDOUT, toRef(e->hash_of_request.container), this));
        return true;
    }
    case msgid::GetContractRSP:
    {
        auto *rs=(MsgData::GetContractRSP*) b.get();
        auto *ctx = it->second.ctx;
        JSScope<20, 20> scope(it->second.ctx);
        auto obj = JS_NewObject(ctx);
        scope.addValue(obj);
        JS_SetPropertyStr(ctx, obj, "found", JS_NewBool(ctx, rs->found()));
        JS_SetPropertyStr(ctx, obj, "errcode", JS_NewInt32(ctx, rs->errcode));
        JS_SetPropertyStr(ctx, obj, "errstr",  JS_NewString(ctx, rs->errstr.c_str()));
        JS_SetPropertyStr(ctx, obj, "contract", JS_NewString(ctx, rs->contract.container.c_str()));
        JSValue owners_arr = JS_NewArray(ctx);
        uint32_t oi = 0;
        for (auto& o : rs->owners) {
            JS_SetPropertyUint32(ctx, owners_arr, oi++, JS_NewString(ctx, base16::encode(o.addr).c_str()));
        }
        JS_SetPropertyStr(ctx, obj, "owners", owners_arr);
        JS_SetPropertyStr(ctx, obj, "src", JS_NewStringLen(ctx, rs->src.data(), rs->src.size()));
        JS_SetPropertyStr(ctx, obj, "ttl", JS_NewInt64(ctx, rs->ttl));
        JS_SetPropertyStr(ctx, obj, "update_time", JS_NewInt64(ctx, rs->update_time));
        JSValue ret = JS_Call(it->second.ctx, it->second.resolve.get(), JS_UNDEFINED, 1, &obj);
        scope.addValue(ret);
        opaque.node_req_promises.erase(e->hash_of_request.container);
        sendEvent(ServiceEnum::Timer, new timerEvent::StopAlarm(Timers::TIMER_ClientMsg_TIMEDOUT, toRef(e->hash_of_request.container), this));
        return true;
    }
    case msgid::GetUserNonceRSP:
    {
        auto *rs=(MsgData::GetUserNonceRSP*) b.get();
        auto *ctx = it->second.ctx;
        JSScope<20, 20> scope(it->second.ctx);
        if (!rs->found()) {
            auto err = JS_NewError(ctx);
            scope.addValue(err);
            JS_DefinePropertyValueStr(ctx, err, "message",
                JS_NewString(ctx, "get_user_nonce: address not found"),
                JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
            JSValue ret = JS_Call(ctx, it->second.reject.get(), JS_UNDEFINED, 1, &err);
            scope.addValue(ret);
            opaque.node_req_promises.erase(e->hash_of_request.container);
            sendEvent(ServiceEnum::Timer, new timerEvent::StopAlarm(Timers::TIMER_ClientMsg_TIMEDOUT, toRef(e->hash_of_request.container), this));
            return true;
        }
        auto obj = JS_NewObject(ctx);
        scope.addValue(obj);
        JS_SetPropertyStr(ctx, obj, "found", JS_NewBool(ctx, rs->found()));
        JS_SetPropertyStr(ctx, obj, "errcode", JS_NewInt32(ctx, rs->errcode));
        JS_SetPropertyStr(ctx, obj, "errstr",  JS_NewString(ctx, rs->errstr.c_str()));        
        JS_SetPropertyStr(ctx, obj, "nonce", JS_NewString(ctx, std::to_string(rs->nonce).c_str()));
        JS_SetPropertyStr(ctx, obj, "balance", JS_NewString(ctx, rs->balance.str().c_str()));
        JSValue ret = JS_Call(it->second.ctx, it->second.resolve.get(), JS_UNDEFINED, 1, &obj);
        scope.addValue(ret);
        opaque.node_req_promises.erase(e->hash_of_request.container);
        sendEvent(ServiceEnum::Timer, new timerEvent::StopAlarm(Timers::TIMER_ClientMsg_TIMEDOUT, toRef(e->hash_of_request.container), this));
        return true;
    }    
    break;
    case msgid::GetNodeInfoRSP:
    {
        auto *rs=(MsgData::GetNodeInfoRSP*) b.get();
        auto *ctx = it->second.ctx;
        JSScope<20, 20> scope(it->second.ctx);
        auto obj = JS_NewObject(ctx);
        scope.addValue(obj);
        JS_SetPropertyStr(ctx, obj, "found", JS_NewBool(ctx, rs->found()));
        JS_SetPropertyStr(ctx, obj, "errcode", JS_NewInt32(ctx, rs->errcode));
        JS_SetPropertyStr(ctx, obj, "errstr",  JS_NewString(ctx, rs->errstr.c_str()));        
        JS_SetPropertyStr(ctx, obj, "name", JS_NewString(ctx, rs->name.container.c_str()));
        JS_SetPropertyStr(ctx, obj, "owner", JS_NewString(ctx, base16::encode(rs->owner.addr).c_str()));
        JS_SetPropertyStr(ctx, obj, "ed_pk", JS_NewString(ctx, rs->ed_pk.c_str()));
        JS_SetPropertyStr(ctx, obj, "bls_pk", JS_NewString(ctx, rs->bls_pk.c_str()));
        JS_SetPropertyStr(ctx, obj, "ip", JS_NewString(ctx, rs->ip.c_str()));
        JS_SetPropertyStr(ctx, obj, "enabled", JS_NewBool(ctx, rs->enabled));
        JS_SetPropertyStr(ctx, obj, "last_update", JS_NewInt64(ctx, rs->last_update));
        JS_SetPropertyStr(ctx, obj, "full_stake", JS_NewString(ctx, rs->full_stake.str().c_str()));
        JSValue stakes_obj = JS_NewObject(ctx);
        // scope.addValue(stakes_obj);
        for (auto& kv : rs->stakes) {
            JS_SetPropertyStr(ctx, stakes_obj, base16::encode(kv.first.addr).c_str(),
                              JS_NewString(ctx, std::to_string(kv.second).c_str()));
        }
        JS_SetPropertyStr(ctx, obj, "stakes", stakes_obj);
        JSValue ret = JS_Call(it->second.ctx, it->second.resolve.get(), JS_UNDEFINED, 1, &obj);
        scope.addValue(ret);
        opaque.node_req_promises.erase(e->hash_of_request.container);
        sendEvent(ServiceEnum::Timer, new timerEvent::StopAlarm(Timers::TIMER_ClientMsg_TIMEDOUT, toRef(e->hash_of_request.container), this));
        return true;
    }
    break;
    case msgid::GetNodesRSP:
    {
        auto *rs = (MsgData::GetNodesRSP*) b.get();
        auto *ctx = it->second.ctx;
        JSScope<20, 20> scope(it->second.ctx);
        auto obj = JS_NewObject(ctx);
        scope.addValue(obj);
        JS_SetPropertyStr(ctx, obj, "found",   JS_NewBool(ctx, rs->found()));
        JS_SetPropertyStr(ctx, obj, "errcode", JS_NewInt32(ctx, rs->errcode));
        JS_SetPropertyStr(ctx, obj, "errstr",  JS_NewString(ctx, rs->errstr.c_str()));
        JSValue nodes_arr = JS_NewArray(ctx);
        int idx = 0;
        for (auto& kv : rs->nodes) {
            JSValue item = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, item, "name", JS_NewString(ctx, kv.first.c_str()));
            JS_SetPropertyStr(ctx, item, "ip",   JS_NewString(ctx, kv.second.c_str()));
            JS_SetPropertyUint32(ctx, nodes_arr, idx++, item);
        }
        JS_SetPropertyStr(ctx, obj, "nodes", nodes_arr);
        JSValue ret = JS_Call(it->second.ctx, it->second.resolve.get(), JS_UNDEFINED, 1, &obj);
        scope.addValue(ret);
        opaque.node_req_promises.erase(e->hash_of_request.container);
        sendEvent(ServiceEnum::Timer, new timerEvent::StopAlarm(Timers::TIMER_ClientMsg_TIMEDOUT, toRef(e->hash_of_request.container), this));
        return true;
    }
    break;
    case msgid::GetValuesRSP:
    {
        auto *rs=(MsgData::GetValuesRSP*) b.get();
        auto *ctx = it->second.ctx;
        JSScope<20, 20> scope(it->second.ctx);
        auto obj = JS_NewObject(ctx);
        scope.addValue(obj);
        JS_SetPropertyStr(ctx, obj, "found", JS_NewBool(ctx, rs->found()));
        JS_SetPropertyStr(ctx, obj, "errcode", JS_NewInt32(ctx, rs->errcode));
        JS_SetPropertyStr(ctx, obj, "errstr",  JS_NewString(ctx, rs->errstr.c_str()));        
        JSValue fees_obj = JS_NewObject(ctx);
        // scope.addValue(fees_obj);
        for (auto& kv : rs->fees) {
            JS_SetPropertyStr(ctx, fees_obj, kv.first.c_str(),
                              JS_NewString(ctx, std::to_string(kv.second).c_str()));
        }
        JS_SetPropertyStr(ctx, obj, "fees", fees_obj);
        JSValue ret = JS_Call(it->second.ctx, it->second.resolve.get(), JS_UNDEFINED, 1, &obj);
        scope.addValue(ret);
        opaque.node_req_promises.erase(e->hash_of_request.container);
        sendEvent(ServiceEnum::Timer, new timerEvent::StopAlarm(Timers::TIMER_ClientMsg_TIMEDOUT, toRef(e->hash_of_request.container), this));
        return true;
    }
    break;
    case msgid::GetContractDataRSP:
    {
        auto *rs=(MsgData::GetContractDataRSP*) b.get();
        auto *ctx = it->second.ctx;
        JSScope<20, 20> scope(it->second.ctx);
        auto obj = JS_NewObject(ctx);
        scope.addValue(obj);
        JS_SetPropertyStr(ctx, obj, "found", JS_NewBool(ctx, rs->found()));
        JS_SetPropertyStr(ctx, obj, "errcode", JS_NewInt32(ctx, rs->errcode));
        JS_SetPropertyStr(ctx, obj, "errstr",  JS_NewString(ctx, rs->errstr.c_str()));        
        JS_SetPropertyStr(ctx, obj, "contract", JS_NewString(ctx, rs->contract.c_str()));
        JS_SetPropertyStr(ctx, obj, "key", JS_NewString(ctx, rs->key.c_str()));
        JS_SetPropertyStr(ctx, obj, "value", mtjs_granule_unpack(ctx, rs->value));
        JSValue ret = JS_Call(it->second.ctx, it->second.resolve.get(), JS_UNDEFINED, 1, &obj);
        scope.addValue(ret);
        opaque.node_req_promises.erase(e->hash_of_request.container);
        sendEvent(ServiceEnum::Timer, new timerEvent::StopAlarm(Timers::TIMER_ClientMsg_TIMEDOUT, toRef(e->hash_of_request.container), this));
        return true;
    }
    break;
    default:
        throw CommonError("ZZZ unhandler %s",msgName(p));
    }
    return false;
}
extern "C" JSValue parse_yyjson(JSContext *ctx, const char *json_str, size_t len);


bool MTJS::Service::ClientTxSubscribeRSP(const bcEvent::ClientTxSubscribeRSP *e)
{
    MUTEX_INSPECTOR;
    XTRY;
#ifdef DEBUG    
    logErr2("[mtjs] ClientTxSubscribeRSP ENTER: tx_hashes=%d tx_wait_promises=%d node_tx_cb=%d",
    (int)e->tx_hashes.size(), (int)opaque.tx_wait_promises.size(), (int)opaque.node_tx_cb.size());
#endif    
    for (auto& kv : opaque.tx_wait_promises) logErr2("  %s", kv.first.c_str());
    if (!e->att_data.valid()) {
        logErr2("ClientTxSubscribeRSP: att_data invalid");
        return true;
    }

    JSScope<10, 10> scope1(js_ctx);

    bool finalized = e->blockAccepted2REQ.valid();
    JSValue js_block = js_block_new(js_ctx, e->blockAccepted2REQ, e->att_data, e->tx_hashes, finalized);
    scope1.addValue(js_block);
    if (JS_IsException(js_block)) {
        logErr2("ClientTxSubscribeRSP: js_block_new exception");
        return true;
    }

    for (size_t ti = 0; ti < e->tx_hashes.size(); ++ti) {

        JSScope<10, 10> scope2(js_ctx);

        auto hex = base16::encode(e->tx_hashes[ti].container);
        auto &th = e->tx_hashes[ti];
#ifdef DEBUG        
        logErr2("[mtjs] ClientTxSubscribeRSP th=%s node_tx_cb.find=%s",
                th.str().c_str(), opaque.node_tx_cb.count(th.container) ? "found" : "not found");
#endif

        // 1) tx_wait_promises (hex) — tx_wait_block
        auto it = opaque.tx_wait_promises.find(hex);
        if (it != opaque.tx_wait_promises.end()) {
            JSValue ret = JS_Call(it->second.ctx, it->second.resolve.get(), JS_UNDEFINED, 1, &js_block);
            scope2.addValue(ret);
            qjs::checkForException(it->second.ctx, ret, "ClientTxSubscribeRSP: resolve tx_wait_promises");
            opaque.tx_wait_promises.erase(it);
            sendEvent(ServiceEnum::Timer, new timerEvent::StopAlarm(Timers::TIMER_TX_WAIT_BLOCK, toRef(hex), this));
        }

        // 2) node_tx_cb (бинарный) — tx_submit(cb) → finalized
        auto cbIt = opaque.node_tx_cb.find(th.container);
        if (cbIt != opaque.node_tx_cb.end()) {
            // logErr2("[mtjs] cb BEFORE JS_Call th=%s", th.str().c_str());
            JSValue obj = JS_NewObject(js_ctx);
            scope2.addValue(obj);
            JS_SetPropertyStr(js_ctx, obj, "tx_hash", JS_NewString(js_ctx, hex.c_str()));
            JS_SetPropertyStr(js_ctx, obj, "status", JS_NewString(js_ctx, "finalized"));
            JSValue ret = JS_Call(js_ctx, cbIt->second.get(), JS_UNDEFINED, 1, &obj);
            scope2.addValue(ret);
            // logErr2("[mtjs] cb AFTER JS_Call th=%s exc=%d", th.str().c_str(), JS_IsException(ret) ? 1 : 0);
            qjs::checkForException(js_ctx, ret, "ClientTxSubscribeRSP: node_tx_cb");
            opaque.node_tx_cb.erase(cbIt);
            sendEvent(ServiceEnum::Timer, new timerEvent::StopAlarm(Timers::TIMER_TX_WAIT_BLOCK, toRef(th.container), this));
        }
    }

    {
        std::string next_key = "next:";
        auto it = opaque.tx_wait_promises.find(next_key);
        if (it != opaque.tx_wait_promises.end()) {
            JSScope<10, 10> scope2(js_ctx);

            JSValue ret = JS_Call(it->second.ctx, it->second.resolve.get(), JS_UNDEFINED, 1, &js_block);
            scope2.addValue(ret);
            qjs::checkForException(it->second.ctx, ret, "ClientTxSubscribeRSP: resolve next");
            opaque.tx_wait_promises.erase(it);
            sendEvent(ServiceEnum::Timer, new timerEvent::StopAlarm(Timers::TIMER_TX_WAIT_BLOCK, toRef(next_key), this));
        }
    }

    if (opaque.tx_subscription_cb.has_value() && JS_IsFunction(js_ctx, opaque.tx_subscription_cb->get())) {
        JSValue ret = JS_Call(js_ctx, opaque.tx_subscription_cb->get(), JS_UNDEFINED, 1, &js_block);
        scope1.addValue(ret);
        qjs::checkForException(js_ctx, ret, "ClientTxSubscribeRSP: tx_subscription_cb");
    }

    XPASS;
    return true;
}