#pragma once
#include "msockaddr_in.h"
#include <string>
#include "jslib/net/http/http_server.h"
#include "common/js_tools.h"
#include "url.hpp"
#include <quickjs.h>
#include <thread>
#include "timerTask.h"
#include "mutexable.h"
#include <set>
#include "IUtils.h"
struct JS_HttpServer;
struct server_conf_base: public Refcountable
{
    enum TYPE
    {
        TYPE_HTTP, TYPE_RPC, TYPE_NONE
    };
    server_conf_base(TYPE t, JSContext*c)
        :typ(t), ctx(c)
    {
        DBG(iUtils->mem_add_ptr("server_conf_base",this));

    }
    TYPE typ;
    JS_HttpServer * js_server;
    JSContext *ctx=nullptr;
    ~server_conf_base()
    {

        DBG(iUtils->mem_remove_ptr("server_conf_base",this));
    }


};
struct server_conf_http: public server_conf_base
{
    /// WS
    std::map<std::string,JSValue> ws_callbacks;
    JSValue callback;

    server_conf_http(JSContext* ctx, const JSValue& cb)
        : server_conf_base(TYPE_HTTP,ctx), callback(cb)
    {
        DBG(iUtils->mem_add_ptr("server_conf_http",this));

    }
    ~server_conf_http()
    {
        DBG(iUtils->mem_remove_ptr("server_conf_http",this));
        for(auto& z:ws_callbacks)
        {
            JS_FreeValue(ctx,z.second);
        }
        JS_FreeValue(ctx,callback);

    }
};

struct RCF: public Refcountable
{
    std::map<msockaddr_in,REF_getter<server_conf_base> > servers;


    struct __timers {
        std::map<int64_t, REF_getter<TimerTask> > timers_refed;
        std::map<int64_t, REF_getter<TimerTask> > timers_unrefed;
    };
    __timers timers;

    JSContext *ctx=nullptr;
    RCF(JSContext* c):ctx(c) {
        DBG(iUtils->mem_add_ptr("RCF",this));

    }
    ~RCF()
    {
        DBG(iUtils->mem_remove_ptr("RCF",this));
        servers.clear();

    }


    std::set<std::string> block_services;


};

