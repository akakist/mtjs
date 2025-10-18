#include "quickjs.h"
#include "mtjsService.h"

JSValue js_rpc_request_new(JSContext *ctx, const REF_getter<mtjsEvent::mtjsRpcREQ>& request, const REF_getter<epoll_socket_info>& esi);
JSValue js_rpc_response_new(JSContext *ctx, const REF_getter<mtjsEvent::mtjsRpcRSP>& response, const REF_getter<epoll_socket_info>& esi);

bool MTJS::Service::mtjsRpcREQ(const mtjsEvent::mtjsRpcREQ* e, const REF_getter<epoll_socket_info>& esi)
{
    XTRY;

    JSScope <10,10> scope(js_ctx);
    auto it=opaque.rpc_on_srv_callbacks.find(e->method);
    if(it==opaque.rpc_on_srv_callbacks.end())
    {
        logErr2("mtjsRpcREQ: no callback for method %s", e->method.c_str());
        return true;
    }
    auto jsreq=js_rpc_request_new(js_ctx, e, esi);
    scope.addValue(jsreq);
    auto ret=JS_Call(js_ctx, it->second.listener, JS_GetGlobalObject(js_ctx), 1, &jsreq);
    scope.addValue(ret);
    qjs::checkForException(js_ctx, ret, "mtjsRpcREQ: JS_Call");

    XPASS;
    return true;
}
bool MTJS::Service::mtjsRpcRSP(const mtjsEvent::mtjsRpcRSP*e, const REF_getter<epoll_socket_info>& esi)
{
    XTRY;
    JSScope <10,10> scope(js_ctx);
    if(e->route.size())
    {
        passEvent(e);
        return true;
    }
    auto it=opaque.rpc_on_cli_callbacks.find(e->method);
    if(it==opaque.rpc_on_cli_callbacks.end())
    {
        logErr2("mtjsRpcRSP: no callback for method %s", e->method.c_str());
        return true;
    }
    auto jsrsp=js_rpc_response_new(js_ctx, e, esi);
    scope.addValue(jsrsp);
    auto ret=JS_Call(js_ctx, it->second.listener, JS_GetGlobalObject(js_ctx), 1, &jsrsp);
    scope.addValue(ret);
    qjs::checkForException(js_ctx, ret, "mtjsRpcRSP: JS_Call");
    XPASS;
    return true;
}
