#include "quickjs.h"
#include "mtjs_opaque.h"
#include "Events/Tools/telnetEvent.h"
static JSValue js_telnet_register_command(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(!op)return JS_EXCEPTION;
    JSScope <10,10> scope(ctx);
    if(argc!=3)
        return JS_ThrowSyntaxError(ctx, "required 4 params");
    for(int i=0; i<argc; i++)
    {
        if(!JS_IsString(argv[i]))
            return JS_ThrowSyntaxError(ctx,"argv %d must be string",i);
    }
    op->broadcaster->sendEvent(ServiceEnum::Telnet, new telnetEvent::RegisterCommand(
                                   std::string(scope.toStdStringView(argv[0])),
                                   std::string(scope.toStdStringView(argv[1])),
                                   std::string(scope.toStdStringView(argv[2])),
                                   op->listener
                               ));
    return JS_UNDEFINED;
}
static JSValue js_telnet_set_callback(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(!op)return JS_EXCEPTION;
    JSScope <10,10> scope(ctx);
    if(argc!=1)
        return JS_ThrowSyntaxError(ctx, "required 1 params");
    if(!JS_IsFunction(ctx,argv[0]))
        return JS_ThrowTypeError(ctx,"arg must be function");
    op->telnet_callback.emplace(JHolder(ctx,argv[0]));
    return JS_UNDEFINED;
}
static JSValue js_telnet_listen(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(!op)return JS_EXCEPTION;
    JSScope <10,10> scope(ctx);
    if(argc!=2)
        return JS_ThrowSyntaxError(ctx, "required 2 params");
    if(!JS_IsString(argv[0]))
        return JS_ThrowTypeError(ctx,"arg 0 must be string addr like localhost:4444");
    if(!JS_IsString(argv[1]))
        return JS_ThrowTypeError(ctx,"arg 1 must be string device name");
    auto addr=scope.toStdStringView(argv[0]);
    auto dn=scope.toStdStringView(argv[1]);
    //op->telnet_callback.emplace(JHolder(ctx,argv[0]));
    msockaddr_in sa;
    sa.init(std::string(addr));
    op->broadcaster->sendEvent(ServiceEnum::Telnet,new telnetEvent::DoListen(sa,std::string(dn),op->listener));
    return JS_UNDEFINED;
}

void register_telnet_request_class(JSContext* ctx);

void init_telnet_object(JSContext *ctx,JSValue & mtjs_obj)
{
    register_telnet_request_class(ctx);

    // Создаём объект telnet
    JSValue telnet_obj = JS_NewObject(ctx);

    JS_SetPropertyStr(ctx, telnet_obj, "register_command",
                      JS_NewCFunction(ctx, js_telnet_register_command, "register_command", 3));
    JS_SetPropertyStr(ctx, telnet_obj, "set_callback",
                      JS_NewCFunction(ctx, js_telnet_set_callback, "set_callback", 1));

    // Добавляем метод listen
    JS_SetPropertyStr(ctx, telnet_obj, "listen",
                      JS_NewCFunction(ctx, js_telnet_listen, "listen", 1));


    JS_SetPropertyStr(ctx, mtjs_obj, "telnet", telnet_obj);
}
