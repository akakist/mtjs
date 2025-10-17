#include "Events/System/Net/httpEvent.h"
#include <quickjs.h>
#include <string>
#include "http_server.h"

#include "msockaddr_in.h"
#include "common/rcf.h"

JSClassID js_http_server_class_id;



static void js_http_server_finalizer(JSRuntime* rt, JSValue val)
{
    MUTEX_INSPECTOR;
    JS_HttpServer* req = static_cast<JS_HttpServer*>(JS_GetOpaque(val, js_http_server_class_id));
    if (req) {
        delete req;
    }
}

static JSClassDef js_http_server_class = {
    "HttpServer",
    .finalizer = js_http_server_finalizer,
};
static JSValue js_http_server_stop(JSContext *ctx, JSValueConst this_val,
                                   int argc, JSValueConst *argv)
{
    JS_HttpServer* js_server = static_cast<JS_HttpServer*>(JS_GetOpaque(this_val, js_http_server_class_id));
    js_server->rcf->servers.erase(js_server->sa);
    return JS_UNDEFINED;

}


static JSValue js_http_server_listen(JSContext *ctx, JSValueConst this_val,
                                     int argc, JSValueConst *argv)
{
    MUTEX_INSPECTOR;
    JS_HttpServer* js_server = static_cast<JS_HttpServer*>(JS_GetOpaque(this_val, js_http_server_class_id));
    // logErr2("js_http_server_listen %p",js_server);
    JSScope scope(ctx);
    int port=0;
    std::string ip="0.0.0.0";
    if(argc==1)
    {
        if(JS_ToInt32(ctx, &port, argv[0]))
        {
            return JS_EXCEPTION;
        }

    }
    else if(argc==2)
    {
        const char * s=scope.toCString(argv[0]);
        if(!s)
            return JS_ThrowTypeError(ctx, "wrong number in arguments2");
        ip=s;

        if(JS_ToInt32(ctx, &port, argv[0]))
        {
            return JS_EXCEPTION;
        }

    }
    else
    {
        return JS_ThrowTypeError(ctx, "wrong number in arguments");
    }
    msockaddr_in sa;
    sa.init(ip,port);
    // logErr2("%s",sa.dump().c_str());
    js_server->sa=sa;

    SECURE ssl;
    ssl.use_ssl=false;
    js_server->brd->sendEvent(ServiceEnum::HTTP, new httpEvent::DoListen(sa,ssl,js_server->lst));
    REF_getter<server_conf_base> c=new server_conf_http(ctx, js_server->http_handler_callback);
    c->js_server= js_server;
    js_server->rcf->servers.insert({sa, c});

    if(!JS_IsFunction(ctx,js_server->http_handler_callback))
    {
        logErr2("if(!JS_IsFunction(ctx,cb)) 2");
    }



    return JS_UNDEFINED;
}
static const JSCFunctionListEntry js_http_server_proto_funcs[] = {
    JS_CFUNC_DEF("listen", 2, js_http_server_listen),
    JS_CFUNC_DEF("stop", 2, js_http_server_stop),

};

void register_http_server_class(JSContext* ctx)
{
    JS_NewClassID(&js_http_server_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_http_server_class_id, &js_http_server_class);

    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_http_server_proto_funcs, sizeof(js_http_server_proto_funcs) / sizeof(JSCFunctionListEntry));

    JS_SetClassProto(ctx, js_http_server_class_id, proto);

}
