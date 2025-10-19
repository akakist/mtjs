#pragma once
#include <REF.h>
#include <SOCKET_id.h>
#include <epoll_socket_info.h>
#include <unknown.h>
#include <listenerBuffered1Thread.h>
#include <broadcaster.h>
#include "event_mt.h"
#include <Events/System/Run/startServiceEvent.h>
#include "Events/System/Net/httpEvent.h"
#include "Events/System/timerEvent.h"
#include "Events/System/Net/socketEvent.h"

#include "quickjs/quickjs.h"
#include "common/rcf.h"
#include "common/mtjs_opaque.h"
#include "common/async_task.h"
#include "common/mtjsEvent.h"
#include "Events/Tools/webHandlerEvent.h"
#include "Events/Tools/telnetEvent.h"

extern "C"
{
}

void dump_memory_usage(JSRuntime* runtime);

extern "C"
{
    JSModuleDef *js_init_module_http(JSContext *ctx, const char *module_name);
}

extern std::set<void*> allocs;

namespace MTJS
{


    class Service:
        public UnknownBase,
        public ListenerBuffered1Thread,
        public Broadcaster
    {


        bool on_startService(const systemEvent::startService*);
        bool handleEvent(const REF_getter<Event::Base>& e);
        bool RequestIncoming(const httpEvent::RequestIncoming*);
        bool CommandEntered(const telnetEvent::CommandEntered*);
        bool WSDisaccepted(const httpEvent::WSDisaccepted*);
        bool WSDisconnected(const httpEvent::WSDisconnected*);
        bool WSTextMessage(const httpEvent::WSTextMessage*);

        bool RequestChunkReceived(const httpEvent::RequestChunkReceived*);
        bool RequestStartChunking(const httpEvent::RequestStartChunking*);
        bool RequestChunkingCompleted(const httpEvent::RequestChunkingCompleted*);

        bool TickTimer(const timerEvent::TickTimer*);
        bool TickAlarm(const timerEvent::TickAlarm*);
        bool Connected(const socketEvent::Connected*);
        bool Disconnected(const socketEvent::Disconnected*);
        bool StreamRead(const socketEvent::StreamRead*);
        bool NotifyOutBufferEmpty(const socketEvent::NotifyOutBufferEmpty*) {
            return true;
        };
        bool AsyncExecuted(const mtjsEvent::AsyncExecuted*);
        bool Eval(const mtjsEvent::Eval*);
        bool EmitterData(const mtjsEvent::EmitterData*);
        bool mtjsRpcREQ(const mtjsEvent::mtjsRpcREQ*,const REF_getter<epoll_socket_info>& esi);
        bool mtjsRpcRSP(const mtjsEvent::mtjsRpcRSP*,const REF_getter<epoll_socket_info>& esi);
#ifdef WEBDUMP
        bool RequestIncoming(const webHandlerEvent::RequestIncoming*);
#endif

        void load_config();
        void executePending();

        void checkForExit();


    private:
        int ticks_request_limit;
    public:
        void stop()
        {
            opaque.async_deque->stop();
            for(auto& t:thread_pool)
            {
                pthread_join(t,NULL);
            }
            thread_pool.clear();
            rconf=nullptr;
            opaque.clear();
            JS_FreeContext(js_ctx);
            JS_FreeRuntime(js_rt);
            // printf("allocs sz %ld\n",allocs.size());

        }
        void deinit()
        {
            ListenerBuffered1Thread::deinit();
            stop();
        }

        Service(const SERVICE_id &svs, const std::string&  nm,IInstance* ifa);
        static UnknownBase* construct(const SERVICE_id& id, const std::string&  nm,IInstance* ifa);
        ~Service();
        IInstance* iInstance;
        time_t start_time=time(NULL);


        REF_getter<RCF> rconf=nullptr;

        JSRuntime* js_rt=nullptr;
        JSContext * js_ctx=nullptr;
        JSValue module__=JS_UNDEFINED;
        // JSValue moduleNamespace__=JS_UNDEFINED;

        mtjs_opaque opaque;



        int64_t timerIdGen=0;

        size_t thread_cnt;
        std::vector<pthread_t> thread_pool;

        double pending_timeout=0;


    };
}
