#pragma once
#include "broadcaster.h"
#include "common/rcf.h"
#include <quickjs/quickjs.h>

struct RCF;
class JS_HttpServer {
public:
    JSValue http_handler_callback;
    ListenerBase* lst;
    Broadcaster* brd;
    REF_getter<RCF> rcf;
    msockaddr_in sa;

    JS_HttpServer(JSContext* ctx, ListenerBase* l, Broadcaster* b, const REF_getter<RCF> &r) : lst(l),brd(b),rcf(r)
    {
    }
    ~JS_HttpServer()
    {
    }
};

