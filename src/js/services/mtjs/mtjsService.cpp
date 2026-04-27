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
#include "version.h"
#include "events_mtjs.hpp"
#include "common/rcf.h"
#include "common/mtjs_opaque.h"
#include <msockaddr_in.h>

#include "common/mtjsEvent.h"
#include "common/jsscope.h"
#include "common/timers.h"
#include "Events/Tools/webHandlerEvent.h"
#include "errcodes.h"

#include "sv.h"
extern "C"
{
}


void* thread_worker(void *p)
{
    mtjs_opaque *op=(mtjs_opaque *)p;
    while(!op->async_deque->m_isTerminating && !iUtils->isTerminating())
    {
        auto x= op->async_deque->pop();
        // for(auto x:d)
        if(x.valid())
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

MTJS::Service::Service(const SERVICE_id& svs, const std::string& nm, IInstance* ifa)
    : UnknownBase(nm),
      ListenerBuffered1Thread(nm, svs, ifa->getConfig()->get_int64_t("STACK_SIZE",8*1024*1024,"thread stack size")),
      Broadcaster(ifa), iInstance(ifa)
{
    try
    {
        XTRY;

        thread_cnt=ifa->getConfig()->get_int64_t("THREAD_POOL_SIZE",10,"js async thread pool count");
        pending_timeout=ifa->getConfig()->get_real("PENDING_TIMEOUT",0.2,"js pending timeout");
        for(int i=0; i<thread_cnt; i++)
        {
            size_t stackSize=8*1024*2014;
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setstacksize(&attr, stackSize);
            XTRY;
            pthread_t pt;
            if(pthread_create(&pt,&attr,thread_worker,&opaque))
                throw CommonError("pthread_create: errno %d",errno);

            thread_pool.push_back(pt);
            XPASS;
        }

        XPASS;
    }
    catch(const std::exception& e)
    {
        logErr2("exception: %s %s %d", e.what(), __FILE__, __LINE__);
        throw;
    }
}

MTJS::Service::~Service()
{
}

UnknownBase* MTJS::Service::construct(const SERVICE_id& id, const std::string& nm, IInstance* ifa)
{
    XTRY;
    return new Service(id, nm, ifa);
    XPASS;
}

#define CONFIG_MODULE_SUPPORT 1
#include <quickjs.h>

void dump_memory_usage(JSRuntime* runtime)
{
    JSMemoryUsage mem_usage;
    JS_ComputeMemoryUsage(runtime, &mem_usage); // Заполняем структуру статистики
    JS_DumpMemoryUsage(stdout, &mem_usage, runtime); // Выводим в консоль
}


void MTJS::Service::load_config()
{
}




void registerMTJSModule(const char* pn)
{
    if(pn) iUtils->registerPlugingInfo(pn, IUtils::PLUGIN_TYPE_SERVICE, ServiceEnum::mtjs, "MTJS", getEvents_mtjs());
    else
    {
        iUtils->registerService(ServiceEnum::mtjs, MTJS::Service::construct, "MTJS");
        regEvents_mtjs();
    }
}

bool MTJS::Service::handleEvent(const REF_getter<Event::Base>& e)
{
    XTRY;
    auto& ID = e->id;

    switch(ID)
    {
    case systemEventEnum::startService :
        return on_startService((const systemEvent::startService*)e.get());
    case timerEventEnum::TickTimer :
        return TickTimer((const timerEvent::TickTimer*)e.get());
    case httpEventEnum::RequestIncoming :
        return RequestIncoming((const httpEvent::RequestIncoming*)e.get());
    case httpEventEnum::RequestStartChunking :
        return RequestStartChunking((const httpEvent::RequestStartChunking*)e.get());
    case httpEventEnum::RequestChunkReceived :
        return RequestChunkReceived((const httpEvent::RequestChunkReceived*)e.get());
    case httpEventEnum::RequestChunkingCompleted :
        return RequestChunkingCompleted((const httpEvent::RequestChunkingCompleted*)e.get());
    case httpEventEnum::WSDisaccepted :
        return WSDisaccepted((const httpEvent::WSDisaccepted*)e.get());
    case httpEventEnum::WSDisconnected :
        return WSDisconnected((const httpEvent::WSDisconnected*)e.get());
    case httpEventEnum::WSTextMessage :
        return WSTextMessage((const httpEvent::WSTextMessage*)e.get());
    case socketEventEnum::Connected :
        return Connected((const socketEvent::Connected*)e.get());
    case socketEventEnum::Disconnected :
        return Disconnected((const socketEvent::Disconnected*)e.get());
    case socketEventEnum::StreamRead :
        return StreamRead((const socketEvent::StreamRead*)e.get());
    case socketEventEnum::NotifyOutBufferEmpty :
        return NotifyOutBufferEmpty((const socketEvent::NotifyOutBufferEmpty*)e.get());
    case timerEventEnum::TickAlarm :
        return TickAlarm((const timerEvent::TickAlarm*)e.get());
    case mtjsEventEnum::AsyncExecuted :
        return AsyncExecuted((const mtjsEvent::AsyncExecuted*)e.get());


    case mtjsEventEnum::Eval :
        return Eval((const mtjsEvent::Eval*)e.get());
    case mtjsEventEnum::EmitterData :
        return EmitterData((const mtjsEvent::EmitterData*)e.get());
    case telnetEventEnum::CommandEntered :
        return CommandEntered((const telnetEvent::CommandEntered*)e.get());
    case bcEventEnum::ClientMsgReply :
        return ClientMsgReply((const bcEvent::ClientMsgReply*)e.get());

    case bcEventEnum::ClientTxSubscribeRSP :
        return ClientTxSubscribeRSP((const bcEvent::ClientTxSubscribeRSP*)e.get());

#ifdef WEBDUMP
    case webHandlerEventEnum::RequestIncoming :
        return RequestIncoming((const webHandlerEvent::RequestIncoming*)e.get());
#endif


    case rpcEventEnum::IncomingOnConnector:
    {
        XTRY;
        const rpcEvent::IncomingOnConnector* E=(const rpcEvent::IncomingOnConnector*)e.get();
        auto &IDC=E->e->id;
        switch (IDC)
        {
        case mtjsEventEnum::mtjsRpcREQ:
            return mtjsRpcREQ((mtjsEvent::mtjsRpcREQ*)E->e.get(),E->esi);
        case mtjsEventEnum::mtjsRpcRSP:
            return mtjsRpcRSP((mtjsEvent::mtjsRpcRSP*)E->e.get(),E->esi);
        case bcEventEnum::ClientMsgReply :
            return ClientMsgReply((const bcEvent::ClientMsgReply*)E->e.get());
        case bcEventEnum::ClientTxSubscribeRSP :
            return ClientTxSubscribeRSP((const bcEvent::ClientTxSubscribeRSP*)E->e.get());

        default:
            logErr2("unhandled event %s %s %d",iUtils->genum_name(E->e->id),__func__,__LINE__);
            break;
        }
        logErr2("unhandled IncomingOnConnector event %s %s %d",iUtils->genum_name(E->e->id),__func__,__LINE__);
        XPASS;
    }
    case rpcEventEnum::IncomingOnAcceptor:
    {
        XTRY;
        const rpcEvent::IncomingOnAcceptor* E=(const rpcEvent::IncomingOnAcceptor*)e.get();
        auto &IDA=E->e->id;
        switch (IDA)
        {
        case mtjsEventEnum::mtjsRpcREQ:
            return mtjsRpcREQ((mtjsEvent::mtjsRpcREQ*)E->e.get(),E->esi);
        case mtjsEventEnum::mtjsRpcRSP:
            return mtjsRpcRSP((mtjsEvent::mtjsRpcRSP*)E->e.get(),E->esi);
        case bcEventEnum::ClientMsgReply :
            return ClientMsgReply((const bcEvent::ClientMsgReply*)E->e.get());
        case bcEventEnum::ClientTxSubscribeRSP :
            return ClientTxSubscribeRSP((const bcEvent::ClientTxSubscribeRSP*)E->e.get());
        default:
            logErr2("unhandled event %s %s %d",iUtils->genum_name(E->e->id),__func__,__LINE__);
            break;
        }
        logErr2("unhandled IncomingOnAcceptor event %s %s %d",iUtils->genum_name(E->e->id),__func__,__LINE__);
        XPASS;
    }

    default:
        logErr2("unhandled event %s",iUtils->genum_name(e->id));
        return false;
    }
    XPASS;
    return false;
}

void MTJS::Service::checkForExit()
{
    executePending();
    if(getPendingCount())
        return;
    if(opaque.rpcBlockExit)
        return;
    {
        M_LOCKC(opaque.async_deque->m_mutex);
        if(opaque.async_deque->container.size()) return;
    }
    if(opaque.rcf->servers.size())
        return;
    if(opaque.rcf->timers.timers_refed.size())
        return;
    if(opaque.rcf->block_services.size())
        return;
    if(!opaque.async_deque->empty())
        return;
    stop();
    exit(0);
}

JSValue makeError(JSContext *ctx, int errid, const char *errstr) {
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "errcode", JS_NewInt32(ctx, errid));
    JS_SetPropertyStr(ctx, obj, "errstr", JS_NewString(ctx, errstr));
    return obj;
}
bool MTJS::Service::TickAlarm(const timerEvent::TickAlarm* e)
{
    MUTEX_INSPECTOR;
    if(e->tid==Timers::TIMER_ClientMsg_TIMEDOUT)
    {
        // logErr2("if(e->tid==Timers::TIMER_ClientMsg_TIMEDOUT)");
        JSScope <10,10> scope(js_ctx);
        THASH_id h;
        h.container=e->data.get()->container;
        auto it=opaque.node_req_promises.find(h);
        if(it==opaque.node_req_promises.end())
            throw CommonError("if(i!=opaque.node_req_promises.end())");
        // it->second.ctx

        JSValue global_obj = JS_GetGlobalObject(js_ctx);
        auto o=makeError(it->second.ctx,BCERR_TIMEDOUT,"timed out");
        scope.addValue(o);
        // logErr2("")
        JSValue func_result = JS_Call(it->second.ctx, it->second.reject, global_obj, 1, &o);
        scope.addValue(func_result);
        opaque.node_req_promises.erase(h);
        return true;
        // JS_Call
    }
    if(e->tid==Timers::TIMER_TIMER)
    {
        JSScope <10,10> scope(js_ctx);
        TimerTask* t=(TimerTask*)e->cookie.get();
        JSValue global_obj = JS_GetGlobalObject(js_ctx);
        scope.addValue(global_obj);


        JSValue func_result = JS_Call(js_ctx, t->cb, global_obj, t->fargs.size(), t->fargs.data());
        scope.addValue(func_result);
        auto id=std::stoll(e->data->container);
        logErr2("TickAlarm: %lld",id);
        rconf->timers.timers_refed.erase(id);
        rconf->timers.timers_unrefed.erase(id);

        return true;
    }
    else throw CommonError("if(e->tid==RCF::TIMER_TIMER)");
    return false;
}
void MTJS::Service::executePending()
{
    for (;;)
    {
        JSContext *ctx1;
        int r = JS_ExecutePendingJob(js_rt, &ctx1);
        if (r < 0)
        {
            fprintf(stderr, "[Loop] JS_ExecutePendingJob => error.\n");
            return;
        }
        if (r == 0) break;

    }

}
bool  MTJS::Service::TickTimer(const timerEvent::TickTimer*e)
{
    MUTEX_INSPECTOR;
    JSScope <10,10> scope(js_ctx);
    if(e->tid==Timers::TIMER_INTERVAL)
    {
        TimerTask* t=(TimerTask*)e->cookie.get();
        logErr2("TickTimer: %lld",std::stoll(e->data->container));
        JSValue global_obj = JS_GetGlobalObject(js_ctx);
        scope.addValue(global_obj);

        JSValue func_result = JS_Call(js_ctx, t->cb, global_obj, t->fargs.size(), t->fargs.data());

        scope.addValue(func_result);
        qjs::checkForException(js_ctx, func_result,"TickTimer: JS_Call");
        return true;
    }
    else if(e->tid==Timers::TIMER_POLL)
    {
        executePending();
        checkForExit();


        return true;
    }
    return false;
}

JSValue loadModule(JSContext* ctx, const std::string& moduleCode)
{
    JSScope <10,10> scope(ctx);
    JSValue module1 = JS_Eval(ctx, moduleCode.c_str(), moduleCode.size(), "<module>", JS_EVAL_TYPE_MODULE | JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_COMPILE_ONLY);

    if(qjs::checkAndPrintException(ctx, module1, "loadModule"))
        return module1;

    JSValue ret = JS_EvalFunction(ctx, module1);
    scope.addValue(ret);
    if(qjs::checkAndPrintException(ctx, ret, "loadModule")) return ret;

    if(JS_IsException(ret))
    {
        JSValue exc_obj = JS_GetException(ctx);
        scope.addValue(exc_obj);
        auto message(scope.toStdStringView(exc_obj));
        JSValue stack_val = JS_GetPropertyStr(ctx, exc_obj, "stack");
        auto stack = JS_IsUndefined(stack_val) ? "No stack available" : scope.toStdStringView(stack_val);

        if(stack.size())
        {
            fprintf(stderr, "Stack trace:\n" SV_FMT "\n", SV_ARG(stack));
        }
        if(message.size())
        {
            fprintf(stderr, "[loadModule] Runtime error: " SV_FMT "\n", SV_ARG(message));
        }
    }
    // #endif
    return module1;
}


bool MTJS::Service::Eval(const mtjsEvent::Eval* e)
{
    module__ = loadModule(js_ctx, e->js);

    executePending();
    checkForExit();

    return true;
}

#ifdef WEBDUMP
bool MTJS::Service::RequestIncoming(const webHandlerEvent::RequestIncoming* e)
{
    auto d=iUtils->mem_dump();
    std::string out;

    for(auto z:d)
    {
        out+=z.first+" "+std::to_string(z.second)+"\n";
    }
    HTTP::Response resp(e->req);
    resp.make_response(out);
    return true;
}
#endif
JSValue create_uint8array(JSContext *ctx, const char *data, size_t len) {
    MUTEX_INSPECTOR;
    JSValue array_buffer = JS_NewArrayBufferCopy(ctx, (const uint8_t*)data, len);
    if (JS_IsException(array_buffer)) {
        return JS_EXCEPTION;
    }
    return array_buffer;
}
bool MTJS::Service::EmitterData(const mtjsEvent::EmitterData*e)
{
    MUTEX_INSPECTOR;

    JSScope <10,10> scope(js_ctx);
    auto sValue = create_uint8array(js_ctx,e->data.data(),e->data.size());
    scope.addValue(sValue);

    e->emitter->emit(e->cmd, 1,&sValue);
    return true;
}
JSValue js_telnet_request_new(JSContext *ctx, const REF_getter<telnetEvent::CommandEntered>& request);

bool MTJS::Service::CommandEntered(const telnetEvent::CommandEntered* e)
{
    MUTEX_INSPECTOR;
    if(opaque.telnet_callback.has_value())
    {
        JSScope <10,10> scope(js_ctx);
        JSValue telnetCallback = opaque.telnet_callback->listener;
        if(JS_IsFunction(js_ctx, telnetCallback))
        {

            JSValue args =  js_telnet_request_new(js_ctx, e) ;
            scope.addValue(args);
            JSValue result = JS_Call(js_ctx, telnetCallback, JS_UNDEFINED, 1, &args);
            scope.addValue(result);
            qjs::checkForException(js_ctx, result, "CommandEntered: JS_Call");
        }
    }
    return true;
}

#include "msg.h"
// #include "quickjspp.hpp"

bool MTJS::Service::ClientMsgReply(const bcEvent::ClientMsgReply*e)
{
    MUTEX_INSPECTOR;

    auto it=opaque.node_req_promises.find(e->hash_of_request);
    if(it==opaque.node_req_promises.end())
        throw CommonError("if(it==opaque.node_req_promises.end())");

    inBuffer in(e->msg);
    auto p=in.get_PN();
    switch(p)
    {
    case msgid::get_user_status_rsp:
    {
        msg::get_user_status_rsp r;
        r.unpack(in);
        auto *ctx=it->second.ctx;
        JSScope<20,20> scope(it->second.ctx);
        auto obj=JS_NewObject(ctx);
        scope.addValue(obj);
        JS_SetPropertyStr(ctx, obj, "balance", JS_NewString(ctx, r.balance.toString().c_str()));
        JS_SetPropertyStr(ctx,obj,"nonce",JS_NewString(ctx, r.nonce.toString().c_str()));
        JSValue ret = JS_Call(it->second.ctx, it->second.resolve, JS_UNDEFINED, 1, &obj);
        scope.addValue(ret);
        opaque.node_req_promises.erase(e->hash_of_request);
        sendEvent(ServiceEnum::Timer, new timerEvent::StopAlarm(Timers::TIMER_ClientMsg_TIMEDOUT,toRef(e->hash_of_request.container),this));
        return true;
    }
    break;
    case msgid::transaction_added_rsp:
    {
        msg::transaction_added_rsp r;
        r.unpack(in);
        auto *ctx=it->second.ctx;
        JSScope<20,20> scope(it->second.ctx);
        auto obj=JS_NewObject(ctx);
        scope.addValue(obj);
        JS_SetPropertyStr(ctx, obj, "err", JS_NewInt32(ctx, r.err));
        JS_SetPropertyStr(ctx,obj,"err_str",JS_NewString(ctx, r.err_str.c_str()));
        JS_SetPropertyStr(ctx,obj,"tx_hash",JS_NewString(ctx, base62::encode(r.tx_hash.container).c_str()));
        JSValue ret = JS_Call(it->second.ctx, it->second.resolve, JS_UNDEFINED, 1, &obj);
        scope.addValue(ret);
        opaque.node_req_promises.erase(e->hash_of_request);
        sendEvent(ServiceEnum::Timer, new timerEvent::StopAlarm(Timers::TIMER_ClientMsg_TIMEDOUT,toRef(e->hash_of_request.container),this));
        return true;
    }
    break;
    case msgid::node_message_ed:
    {
        msg::node_message_ed nm;
        nm.unpack(in);
        inBuffer inn(nm.payload);
        auto pp=inn.get_PN();
        switch(pp)
        {
            break;

        }
        return true;
    }
    break;
    default:
        logErr2("unhandled msg Z %d",p);
    }
    return false;
}
#include <nlohmann/json.hpp>
// JSValue convert_yyjson_to_js(JSContext *ctx, yyjson_val *val);
extern "C"
JSValue parse_yyjson(JSContext *ctx, const char *json_str, size_t len);

bool MTJS::Service::ClientTxSubscribeRSP(const bcEvent::ClientTxSubscribeRSP* e)
{
    MUTEX_INSPECTOR;
    // logErr2("ClientTxSubscribeRSP from node");

    msg::publish_block pb(e->msg);
    nlohmann::json j;
    for(size_t ti=0; ti<pb.att_data.trs.size(); ti++)
    {
        nlohmann::json jtr;
        if(opaque.tx_subscription_cb.has_value())
        {
            THASH_id tx_hash=blake2b_hash(pb.att_data.trs[ti].container);
            jtr["tx_hash"] = base62::encode(tx_hash.container);
            // logErr2("ClientTxSubscribeRSP: tx_hash %s",base62::encode(tx_hash.container).c_str());
            JSScope <10,10> scope(js_ctx);
            JSValue global_obj = JS_GetGlobalObject(js_ctx);
            scope.addValue(global_obj);

            for(int ii=0; ii<pb.att_data.instruction_reports[ti].size(); ii++)
            {
                nlohmann::json ii_json;
                ii_json["errcode"]=pb.att_data.instruction_reports[ti][ii].err_code;
                ii_json["errstr"]=pb.att_data.instruction_reports[ti][ii].err_str;
                // logErr2("err_str %d %d %s",ti,ii,pb.att_data.instruction_reports[ti][ii].err_str.c_str());
                nlohmann::json logmsgs_json;
                for(int k=0; k<pb.att_data.instruction_reports[ti][ii].logMsgs.size(); k++)
                {
                    logErr2("logmsgs %d %d %d %s",ti,ii,k,pb.att_data.instruction_reports[ti][ii].logMsgs[k].c_str());
                    logmsgs_json.push_back( pb.att_data.instruction_reports[ti][ii].logMsgs[k]);
                }
                ii_json["logMsgs"]=logmsgs_json;
                jtr["instructions"].push_back(ii_json);

            }
            if(!JS_IsFunction(js_ctx,opaque.tx_subscription_cb->listener))
            {
                throw CommonError("callback not a function");
            }
            auto str=jtr.dump();
            // logErr2("json %s",str.c_str());
            JSValue obj = JS_ParseJSON(js_ctx, str.data(), str.size(), "<input>");
            scope.addValue(obj);
            JSValue argv[1];
            argv[0] = obj;
            JSValue func_result = JS_Call(js_ctx, opaque.tx_subscription_cb->listener, global_obj, 1, argv);
            scope.addValue(func_result);
            qjs::checkForException(js_ctx, func_result, "ClientTxSubscribeRSP: JS_Call");
        }
    }
    return true;
}
