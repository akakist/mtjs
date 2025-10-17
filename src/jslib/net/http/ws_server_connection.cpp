#include <quickjs.h>
#include "jslib/net/http/ws_server_connection.h"
#include "Events/System/Net/httpEvent.h"

JSClassID js_ws_server_connection_class_id;



static void js_ws_server_connection_finalizer(JSRuntime* rt, JSValue val) {

    JS_WS_server_connection* req = static_cast<JS_WS_server_connection*>(JS_GetOpaque(val, js_ws_server_connection_class_id));
    if (req) {

        delete req;
    }
}

static JSClassDef js_ws_server_connection_class = {
    "ws_connection",
    .finalizer = js_ws_server_connection_finalizer,
};



static JSValue js_ws_server_connection_on(JSContext *ctx, JSValueConst this_val,
        int argc, JSValueConst *argv)

{
    JSScope scope(ctx);
    JS_WS_server_connection* conn = static_cast<JS_WS_server_connection*>(JS_GetOpaque2(ctx, this_val, js_ws_server_connection_class_id));
    if(conn==NULL)
        return JS_ThrowInternalError(ctx,"if(conn==NULL)");
    if(argc!=2)
        return JS_ThrowInternalError(ctx,"if(argc!=2)");
    if(!JS_IsString(argv[0]))
        return JS_ThrowInternalError(ctx,"if(!JS_IsString(ctx,argv[0]))");
    if(!JS_IsFunction(ctx,argv[1]))
        return JS_ThrowInternalError(ctx,"if(!JS_IsFunction(ctx,argv[1]))");


    auto key=scope.toStdString(argv[0]);
    conn->emitter->on(key,argv[1]);


    return JS_UNDEFINED;

}

static JSValue js_ws_server_connection_send(JSContext *ctx, JSValueConst this_val,
        int argc, JSValueConst *argv)

{
    JSScope scope(ctx);
    JS_WS_server_connection* conn = static_cast<JS_WS_server_connection*>(JS_GetOpaque2(ctx, this_val, js_ws_server_connection_class_id));
    if(conn==NULL)
        return JS_ThrowInternalError(ctx,"if(conn==NULL)");
    if(argc!=1)
        return JS_ThrowInternalError(ctx,"if(argc!=1)");
    auto msg=scope.toStdString(argv[0]);

    conn->broadcaster->sendEvent(ServiceEnum::HTTP, new httpEvent::WSWrite(conn->req,msg,conn->listener));


    return JS_UNDEFINED;

}

static JSValue js_emitter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    JS_WS_server_connection* req = static_cast<JS_WS_server_connection*>(JS_GetOpaque2(ctx, this_val, js_ws_server_connection_class_id));
    if(!req) return JS_ThrowInternalError(ctx, "[js_emitter] if(!req)");
    if(argc!=0) return JS_ThrowTypeError(ctx, "[js_emitter] number of argument must be 0");
    return new_event_emitter(ctx,req->emitter);
}



static const JSCFunctionListEntry js_ws_server_connection_proto_funcs[] = {
    JS_CFUNC_DEF("on", 2, js_ws_server_connection_on),
    JS_CFUNC_DEF("send", 1, js_ws_server_connection_send),
    JS_CFUNC_DEF("emitter", 1, js_emitter),

};

void register_ws_server_connection_class(JSContext* ctx)
{
    JS_NewClassID(&js_ws_server_connection_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_ws_server_connection_class_id, &js_ws_server_connection_class);

    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_ws_server_connection_proto_funcs, sizeof(js_ws_server_connection_proto_funcs) / sizeof(JSCFunctionListEntry));

    JS_SetClassProto(ctx, js_ws_server_connection_class_id, proto);

}
