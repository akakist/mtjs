#include <quickjs.h>
#include <string>
#include "common/js_tools.h"
#include "mtjs_opaque.h"
#include "Events/Tools/telnetEvent.h"


static JSClassID js_telnet_request_class_id;

class JS_telnet_Request {
public:
    REF_getter<telnetEvent::CommandEntered> req=nullptr;

    JS_telnet_Request(const REF_getter<telnetEvent::CommandEntered>& _req ) : req(_req)
    {
    }
    ~JS_telnet_Request()
    {
    }

};


static void js_request_finalizer(JSRuntime* rt, JSValue val) {
    JS_telnet_Request* req = static_cast<JS_telnet_Request*>(JS_GetOpaque(val, js_telnet_request_class_id));
    if (req) {
        delete req;
    }
}

static JSClassDef js_telnet_request_class = {
    "telnet_request",
    .finalizer = js_request_finalizer,
};

static JSValue js_request_get_command(JSContext* ctx, JSValueConst this_val)
{
    XTRY;
    MUTEX_INSPECTOR;
    JS_telnet_Request* req = static_cast<JS_telnet_Request*>(JS_GetOpaque2(ctx, this_val, js_telnet_request_class_id));
    if (!req) return JS_EXCEPTION;
    if(req->req->command.empty())
        return JS_ThrowRangeError(ctx,"command undefined");
    return JS_NewStringLen(ctx,req->req->command.data(),req->req->command.size());
    XPASS;
}
static JSValue js_request_get_path(JSContext* ctx, JSValueConst this_val)
{
    XTRY;
    MUTEX_INSPECTOR;
    JS_telnet_Request* req = static_cast<JS_telnet_Request*>(JS_GetOpaque2(ctx, this_val, js_telnet_request_class_id));
    if (!req) return JS_EXCEPTION;

    return JS_NewStringLen(ctx,req->req->path.data(),req->req->path.size());
    XPASS;
}



JSValue js_telnet_request_print(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    XTRY;
    JSScope<10,10> scope(ctx);
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(op==NULL)
    {
        printf("if(op==NULL)\n");
        return JS_ThrowTypeError(ctx, "if(op==NULL)");
    }

    JS_telnet_Request* req = static_cast<JS_telnet_Request*>(JS_GetOpaque2(ctx, this_val, js_telnet_request_class_id));
    if (!req) return JS_EXCEPTION;

    std::string buf;
    // logErr2("argc %d",argc);
    for(int i=0; i<argc; i++)
    {
        if(JS_IsString(argv[i]))
        {

            buf+=scope.toStdStringView(argv[i]);

        }
        else if(JS_IsObject(argv[i]))
        {
            std::string res;
            qjs::convert_js_value_to_json(ctx,argv[i],res);
            buf+=res;
        }
        else if(JS_IsNumber(argv[i]))
        {
            JSValue str_val = JS_ToString(ctx, argv[i]);
            scope.addValue(str_val);
            if (JS_IsException(str_val)) {
                fprintf(stderr, "Error converting number to string\n");
                return JS_ThrowTypeError(ctx,"Error converting number to string");
            }
            buf+=scope.toStdStringView(str_val);
        }
        else
            return JS_ThrowSyntaxError(ctx,"invalid print object");

    }
    op->broadcaster->sendEvent(ServiceEnum::Telnet,new telnetEvent::Reply(req->req->socketId,buf,op->listener));
    return JS_UNDEFINED;
    XPASS;

    return JS_UNDEFINED;


}


static const JSCFunctionListEntry js_request_proto_funcs[] = {
    JS_CFUNC_DEF("print", 1, js_telnet_request_print),
    JS_CGETSET_DEF("command", js_request_get_command, NULL),
    JS_CGETSET_DEF("path", js_request_get_path, NULL),
};

void register_telnet_request_class(JSContext* ctx)
{
    XTRY;
    MUTEX_INSPECTOR;
    JS_NewClassID(&js_telnet_request_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_telnet_request_class_id, &js_telnet_request_class);

    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_request_proto_funcs, sizeof(js_request_proto_funcs) / sizeof(JSCFunctionListEntry));

    JS_SetClassProto(ctx, js_telnet_request_class_id, proto);
    XPASS;

}


JSValue js_telnet_request_new(JSContext *ctx, const REF_getter<telnetEvent::CommandEntered>& request)
{
    MUTEX_INSPECTOR;
    XTRY;

    JS_telnet_Request* req = new JS_telnet_Request(request);
    JSValue jsReq = JS_NewObjectClass(ctx,::js_telnet_request_class_id);

    qjs::checkForException(ctx,jsReq,"RequestIncoming: JS_NewObjectClass");
    JS_SetOpaque(jsReq, req);
    return jsReq;
    XPASS;
}
