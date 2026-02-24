#pragma once
#include "httpConnection.h"
#include <quickjs.h>
#include "broadcaster.h"
#include "eventEmitter.h"


class JS_WS_server_connection {
public:
    ListenerBase *listener;
    Broadcaster * broadcaster;
    REF_getter<HttpContext> req;
    REF_getter<EventEmitter> emitter;
    JSContext *ctx=nullptr;
    JS_WS_server_connection(JSContext *c, const REF_getter<HttpContext>& r, ListenerBase *l, Broadcaster *b) : ctx(c), req(r),
        listener(l),broadcaster(b),
        emitter(new EventEmitter(c,"JS_WS_server_connection"))
    {
    }
    ~JS_WS_server_connection()
    {
    }

};
