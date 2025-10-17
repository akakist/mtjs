#include <quickjs.h>
#include <string>
#include "common/js_tools.h"
#include "malloc_debug.h"
#include "mtjsEvent.h"
#include "mtjs_opaque.h"
#include "tools_mt.h"

static JSClassID js_rpc_request_class_id;

class JS_RPC_Request {
public:
    REF_getter<mtjsEvent::mtjsRpcREQ> req=nullptr;
    REF_getter<epoll_socket_info> esi=nullptr;

    JS_RPC_Request(const REF_getter<mtjsEvent::mtjsRpcREQ>& _req, const REF_getter<epoll_socket_info>& _esi ) : req(_req),esi(_esi)
    {
        DBG(iUtils->mem_add_ptr("JS_RPC_Request",this));

        DBG(logErr2("JS_RPC_Request()"));
    }
    ~JS_RPC_Request()
    {
        DBG(iUtils->mem_remove_ptr("JS_RPC_Request",this));
        DBG(logErr2("~JS_RPC_Request()"));

    }

};


static void js_request_finalizer(JSRuntime* rt, JSValue val) {
    JS_RPC_Request* req = static_cast<JS_RPC_Request*>(JS_GetOpaque(val, js_rpc_request_class_id));
    if (req) {
        delete req;
    }
}

static JSClassDef js_rpc_request_class = {
    "rpc_request",
    .finalizer = js_request_finalizer,
};



static JSValue js_request_get_peer_name(JSContext* ctx, JSValueConst this_val)
{
    XTRY;
    MUTEX_INSPECTOR;
    JS_RPC_Request* req = static_cast<JS_RPC_Request*>(JS_GetOpaque2(ctx, this_val, js_rpc_request_class_id));
    if (!req) return JS_EXCEPTION;
    auto nm=JS_NewObject(ctx);
    DBG(memctl_add_object(nm, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));
    auto host=req->esi->remote_name().getStringAddr();
    auto port=req->esi->remote_name().port();
    JS_DefinePropertyValueStr(ctx, nm, "host", JS_NewStringLen(ctx, host.data(), host.size()), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, nm, "port", JS_NewInt32(ctx, port), JS_PROP_C_W_E);
    return nm;
    XPASS;
}

static JSValue js_request_get_params(JSContext* ctx, JSValueConst this_val)
{
    MUTEX_INSPECTOR;
    XTRY;
    JS_RPC_Request* req = static_cast<JS_RPC_Request*>(JS_GetOpaque2(ctx, this_val, js_rpc_request_class_id));

    if (!req) return JS_EXCEPTION;

    return qjs::json_to_jsobject(ctx, req->req->params);
    XPASS;
}
JSValue js_rpc_request_reply(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    XTRY;
    JSScope scope(ctx);
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(op==NULL)
    {
        printf("if(op==NULL)\n");
        return JS_ThrowTypeError(ctx, "if(op==NULL)");
    }

    JS_RPC_Request* req = static_cast<JS_RPC_Request*>(JS_GetOpaque2(ctx, this_val, js_rpc_request_class_id));
    if (!req) return JS_EXCEPTION;

    if (argc != 2) {
        return JS_ThrowTypeError(ctx, "reply expects 2 args: method, params");
    }
    if (!JS_IsString(argv[0]))
        return JS_ThrowTypeError(ctx, "reply expects a string method");

    if (!JS_IsObject(argv[1]))
        return JS_ThrowTypeError(ctx, "reply expects an object params");

    auto method = scope.toStdString(argv[0]);
    if (method.empty())
        return JS_ThrowTypeError(ctx, "method must not be empty");

    std::string params;
    qjs::convert_js_value_to_json(ctx, argv[1],params);

    op->broadcaster->passEvent(new mtjsEvent::mtjsRpcRSP(
                                   std::move(method),
                                   std::move(params),
                                   poppedFrontRoute(req->req->route)
                               ));


    XPASS;

    return JS_UNDEFINED;


}
JSValue js_rpc_request_sendTo(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    XTRY;
    JSScope scope(ctx);
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(op==NULL)
    {
        printf("if(op==NULL)\n");
        return JS_ThrowTypeError(ctx, "if(op==NULL)");
    }

    JS_RPC_Request* req = static_cast<JS_RPC_Request*>(JS_GetOpaque2(ctx, this_val, js_rpc_request_class_id));
    if (!req) return JS_EXCEPTION;

    if (argc != 3) {
        return JS_ThrowTypeError(ctx, "reply expects 3 args: addr, method, params");
    }
    if (!JS_IsString(argv[0]))
        return JS_ThrowTypeError(ctx, "reply expects a string address like 192.168.1.2:8080");

    if (!JS_IsString(argv[1]))
        return JS_ThrowTypeError(ctx, "reply expects a string method");

    if (!JS_IsObject(argv[2]))
        return JS_ThrowTypeError(ctx, "reply expects an object params");

    auto addr = scope.toStdString(argv[0]);
    if (addr.empty())
        return JS_ThrowTypeError(ctx, "address must not be empty");
    auto method = scope.toStdString(argv[1]);
    if (method.empty())
        return JS_ThrowTypeError(ctx, "method must not be empty");

    std::string params;
    qjs::convert_js_value_to_json(ctx, argv[2],params);

    op->broadcaster->sendEvent(addr,ServiceEnum::mtjs, new mtjsEvent::mtjsRpcREQ(
                                   std::move(method),
                                   std::move(params),
                                   req->req->route
                               ));



    XPASS;
    return JS_UNDEFINED;


}



static const JSCFunctionListEntry js_request_proto_funcs[] = {
    JS_CFUNC_DEF("reply", 1, js_rpc_request_reply),
    JS_CFUNC_DEF("sendTo", 1, js_rpc_request_sendTo),
    JS_CGETSET_DEF("peer_name", js_request_get_peer_name, NULL),
    JS_CGETSET_DEF("params", js_request_get_params, NULL),
};

void register_rpc_request_class(JSContext* ctx)
{
    XTRY;
    MUTEX_INSPECTOR;
    JS_NewClassID(&js_rpc_request_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_rpc_request_class_id, &js_rpc_request_class);

    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_request_proto_funcs, sizeof(js_request_proto_funcs) / sizeof(JSCFunctionListEntry));

    JS_SetClassProto(ctx, js_rpc_request_class_id, proto);
    XPASS;

}


JSValue js_rpc_request_new(JSContext *ctx, const REF_getter<mtjsEvent::mtjsRpcREQ>& request, const REF_getter<epoll_socket_info>& esi)
{
    MUTEX_INSPECTOR;
    XTRY;

    JS_RPC_Request* req = new JS_RPC_Request(request,esi);
    JSValue jsReq = JS_NewObjectClass(ctx,::js_rpc_request_class_id);
    DBG(memctl_add_object(jsReq, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

    qjs::checkForException(ctx,jsReq,"RequestIncoming: JS_NewObjectClass");
    JS_SetOpaque(jsReq, req);
    return jsReq;
    XPASS;
}
