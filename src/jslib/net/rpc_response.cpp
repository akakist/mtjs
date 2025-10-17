#include <quickjs.h>
#include <string>
#include "common/js_tools.h"
#include "malloc_debug.h"
#include "mtjsEvent.h"
#include "epoll_socket_info.h"

static JSClassID js_rpc_response_class_id;

class JS_RPC_response {
public:
    REF_getter<mtjsEvent::mtjsRpcRSP> rsp=nullptr;
    REF_getter<epoll_socket_info> esi=nullptr;

    JS_RPC_response(const REF_getter<mtjsEvent::mtjsRpcRSP>& _rsp, const REF_getter<epoll_socket_info>& _esi ) : rsp(_rsp),esi(_esi)
    {
        DBG(iUtils->mem_add_ptr("JS_RPC_response",this));

        DBG(logErr2("JS_RPC_response()"));
    }
    ~JS_RPC_response()
    {
        DBG(iUtils->mem_remove_ptr("JS_RPC_response",this));
        DBG(logErr2("~JS_RPC_response()"));

    }

};


static void js_response_finalizer(JSRuntime* rt, JSValue val) {
    JS_RPC_response* req = static_cast<JS_RPC_response*>(JS_GetOpaque(val, js_rpc_response_class_id));
    if (req) {
        delete req;
    }
}

static JSClassDef js_rpc_response_class = {
    "rpc_response",
    .finalizer = js_response_finalizer,
};



static JSValue js_response_get_peer_name(JSContext* ctx, JSValueConst this_val)
{
    MUTEX_INSPECTOR;
    XTRY;
    JS_RPC_response* req = static_cast<JS_RPC_response*>(JS_GetOpaque2(ctx, this_val, js_rpc_response_class_id));
    if (!req) return JS_EXCEPTION;
    auto nm=JS_NewObject(ctx);
    DBG(memctl_add_object(nm, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));
    auto host=req->esi->remote_name().getStringAddr();
    auto port=req->esi->remote_name().port();
    JS_DefinePropertyValueStr(ctx, nm, "host", JS_NewStringLen(ctx, host.data(), host.size()), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, nm, "port", JS_NewInt32(ctx, port), JS_PROP_C_W_E);
    // JS_DefinePropertyValueStr
    return nm;
    XPASS;
}

static JSValue js_response_get_params(JSContext* ctx, JSValueConst this_val)
{
    MUTEX_INSPECTOR;
    JS_RPC_response* req = static_cast<JS_RPC_response*>(JS_GetOpaque2(ctx, this_val, js_rpc_response_class_id));

    if (!req) return JS_EXCEPTION;

    return qjs::json_to_jsobject(ctx, req->rsp->params);
}


static const JSCFunctionListEntry js_response_proto_funcs[] = {
    JS_CGETSET_DEF("peer_name", js_response_get_peer_name, NULL),
    JS_CGETSET_DEF("params", js_response_get_params, NULL),
};

void register_rpc_response_class(JSContext* ctx)
{
    MUTEX_INSPECTOR;
    JS_NewClassID(&js_rpc_response_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_rpc_response_class_id, &js_rpc_response_class);

    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_response_proto_funcs, sizeof(js_response_proto_funcs) / sizeof(JSCFunctionListEntry));

    JS_SetClassProto(ctx, js_rpc_response_class_id, proto);

}


JSValue js_rpc_response_new(JSContext *ctx, const REF_getter<mtjsEvent::mtjsRpcRSP>& response, const REF_getter<epoll_socket_info>& esi)
{
    MUTEX_INSPECTOR;

    JS_RPC_response* req = new JS_RPC_response(response,esi);
    JSValue jsReq = JS_NewObjectClass(ctx,::js_rpc_response_class_id);
    DBG(memctl_add_object(jsReq, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

    qjs::checkForException(ctx,jsReq,"ResponseIncoming: JS_NewObjectClass");
    JS_SetOpaque(jsReq, req);
    return jsReq;
}
