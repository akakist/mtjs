#include "quickjs.h"
#include "common/mtjs_opaque.h"
#include "http_server.h"
#include "malloc_debug.h"



#define countof(x) (sizeof(x) / sizeof((x)[0]))



extern JSClassID js_http_server_class_id;

static JSValue js_createServer(JSContext *ctx, JSValueConst this_val,
                               int argc, JSValueConst *argv)
{
    MUTEX_INSPECTOR;
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(op==NULL)
    {
        printf("if(op==NULL)\n");
        return JS_ThrowTypeError(ctx, "if(op==NULL)");
    }

    if(argc<1)
    {
        logErr2("if(argc<1)");
        return JS_ThrowTypeError(ctx, "if(argc<1) js_createServer");
    }


    JSValue cb=JS_UNDEFINED;
    if(argc==1)
    {
        cb=JS_DupValue(ctx,argv[0]);
    }
    else
    {
        return JS_ThrowTypeError(ctx, "wrong number in arguments");
    }
    if(!JS_IsFunction(ctx,cb))
    {
        logErr2("if(!JS_IsFunction(ctx,cb))");
        return JS_EXCEPTION;
    }


    JS_HttpServer* srv = new JS_HttpServer(ctx, op->listener,op->broadcaster,op->rcf);
    JSValue jsSrv = JS_NewObjectClass(ctx,js_http_server_class_id);

    if (JS_IsException(jsSrv)) {
        logErr2("if (JS_IsException(jsReq)) {");
        return JS_ThrowTypeError(ctx,"error JS_NewObjectClass js_http_server_class_id");
    }
    srv->http_handler_callback=cb;

    JS_SetOpaque(jsSrv, srv);

    return jsSrv;
}


static const JSCFunctionListEntry js_mthttp_funcs[] = {
    JS_CFUNC_DEF("createServer", 1, js_createServer ),

    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "http", JS_PROP_CONFIGURABLE ),
};

static const JSCFunctionListEntry js_mthttp_obj[] = {
    JS_OBJECT_DEF("http", js_mthttp_funcs, countof(js_mthttp_funcs), JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE ),
};
extern "C"
void add_mt_http_module(JSContext *ctx,JSValue & mtjs_obj)
{
    JS_SetPropertyFunctionList(ctx,mtjs_obj,js_mthttp_obj, countof(js_mthttp_obj));
    // JS_SetPropertyFunctionListGlobal(ctx, js_mthttp_obj, countof(js_mthttp_obj));
}


