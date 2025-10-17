#include <quickjs.h>
#include <string>
#include <vector>
#include "common/mtjs_opaque.h"
#include "common/jsscope.h"
#include "Events/System/Net/rpcEvent.h"
#include "js_tools.h"
#include "mtjsEvent.h"

// Объявляем прототипы функций для методов
static JSValue js_rpc_listen(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
static JSValue js_rpc_on(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);

// Структура для хранения обработчиков событий
struct EventHandler {
    std::string event;
    JSValue callback; // Храним JS-функцию
};

// Контекст для хранения обработчиков
struct RPCContext {
    std::vector<EventHandler> handlers;
};

static JSValue js_rpc_sendTo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    JSScope scope(ctx);
    if(op==NULL)
    {
        printf("if(op==NULL)\n");
        return JS_ThrowTypeError(ctx, "if(op==NULL)");
    }

    if (argc != 3) {
        return JS_ThrowTypeError(ctx, "sendTo expects exactly 3 argument dstAddr, method, params");
    }
    if (!JS_IsString(argv[0])) {
        return JS_ThrowTypeError(ctx, "sendTo expects a string dstAddr");
    }
    if(!JS_IsString(argv[1])) {
        return JS_ThrowTypeError(ctx, "sendTo expects a string method");
    }
    if(!JS_IsObject(argv[2])) {
        return JS_ThrowTypeError(ctx, "sendTo expects an object params");
    }

    auto dstAddr = scope.toStdString(argv[0]);
    if (dstAddr.empty()) {
        return JS_ThrowTypeError(ctx, "dstAddr must not be empty");
    }
    auto method = scope.toStdString(argv[1]);
    if (method.empty()) {
        return JS_ThrowTypeError(ctx, "method must not be empty");
    }
    std::string params;
    qjs::convert_js_value_to_json(ctx,argv[2],params);


    op->broadcaster->sendEvent(dstAddr,ServiceEnum::mtjs, new mtjsEvent::mtjsRpcREQ(std::move(method), std::move(params),op->listener->serviceId));

    op->rpcBlockExit=true; // Блокируем выход, пока слушаем RPC


    return JS_UNDEFINED;
}


// Реализация метода listen(JObject)
static JSValue js_rpc_stop(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(op==NULL)
    {
        printf("if(op==NULL)\n");
        return JS_ThrowTypeError(ctx, "if(op==NULL)");
    }
    op->rpcBlockExit=false; // Блокируем выход, пока слушаем RPC
    return JS_UNDEFINED;

}

static JSValue js_rpc_listen(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{

    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    JSScope scope(ctx);
    if(op==NULL)
    {
        printf("if(op==NULL)\n");
        return JS_ThrowTypeError(ctx, "if(op==NULL)");
    }

    if (argc != 1) {
        return JS_ThrowTypeError(ctx, "listen expects exactly 1 argument");
    }

    // Проверяем, что аргумент — объект
    if (!JS_IsObject(argv[0])) {
        return JS_ThrowTypeError(ctx, "listen expects an object");
    }

    // Пример: получаем свойство из объекта (например, config.port)
    JSValue ip_val = JS_GetPropertyStr(ctx, argv[0], "ip");
    scope.addValue(ip_val);

    JSValue ssl_val = JS_GetPropertyStr(ctx, argv[0], "ssl");
    scope.addValue(ssl_val);
    SECURE sec;
    if(JS_IsBool(ssl_val))
    {
        bool use_ssl = JS_ToBool(ctx, ssl_val);
        if(use_ssl)
        {
            sec.use_ssl=true;
            JSValue cert_val = JS_GetPropertyStr(ctx, argv[0], "ssl_cert");
            scope.addValue(cert_val);
            if(JS_IsString(cert_val))
            {
                sec.cert_pn=scope.toStdString(cert_val);
            }
            JSValue key_val = JS_GetPropertyStr(ctx, argv[0], "ssl_key");
            scope.addValue(key_val);
            if(JS_IsString(key_val))
            {
                sec.key_pn = scope.toStdString(key_val);
            }
        }
    }

    if (!JS_IsUndefined(ip_val)) {
        auto ip= scope.toStdString(ip_val);
        msockaddr_in sa;
        sa.init(ip);
        printf("RPC listening port %s\n", ip.c_str());
        op->broadcaster->sendEvent(ServiceEnum::RPC, new rpcEvent::DoListen(sa,sec));
        op->rpcBlockExit=true; // Блокируем выход, пока слушаем RPC
    }
    return JS_DupValue(ctx, this_val);
}

// Реализация метода on(string, callback)

static JSValue js_rpc_on_server(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{

    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    JSScope scope(ctx);
    if(op==NULL)
    {
        printf("if(op==NULL)\n");
        return JS_ThrowTypeError(ctx, "if(op==NULL)");
    }

    if (argc != 2) {
        return JS_ThrowTypeError(ctx, "on expects exactly 2 arguments method, callback");
    }

    // Проверяем, что первый аргумент — строка
    if (!JS_IsString(argv[0])) {
        return JS_ThrowTypeError(ctx, "method arg must be a string");
    }

    // Проверяем, что второй аргумент — функция
    if (!JS_IsFunction(ctx, argv[1])) {
        return JS_ThrowTypeError(ctx, "callback must be a function");
    }

    auto method = scope.toStdString(argv[0]);

    op->rpc_on_srv_callbacks.insert({method,JHolder(ctx, argv[1])});

    return JS_UNDEFINED;
}
static JSValue js_rpc_on_client(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{

    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    JSScope scope(ctx);
    if(op==NULL)
    {
        printf("if(op==NULL)\n");
        return JS_ThrowTypeError(ctx, "if(op==NULL)");
    }

    if (argc != 2) {
        return JS_ThrowTypeError(ctx, "on expects exactly 2 arguments method, callback");
    }

    // Проверяем, что первый аргумент — строка
    if (!JS_IsString(argv[0])) {
        return JS_ThrowTypeError(ctx, "method arg must be a string");
    }

    // Проверяем, что второй аргумент — функция
    if (!JS_IsFunction(ctx, argv[1])) {
        return JS_ThrowTypeError(ctx, "callback must be a function");
    }

    auto method = scope.toStdString(argv[0]);

    op->rpc_on_cli_callbacks.insert({method,JHolder(ctx, argv[1])});

    return JS_UNDEFINED;
}

// Создаём и регистрируем объект rpc
void init_rpc_object(JSContext *ctx,JSValue & mtjs_obj)
{
    // Создаём объект rpc
    JSValue rpc_obj = JS_NewObject(ctx);

    JS_SetPropertyStr(ctx, rpc_obj, "stop",
                      JS_NewCFunction(ctx, js_rpc_stop, "stop", 1));

    // Добавляем метод listen
    JS_SetPropertyStr(ctx, rpc_obj, "listen",
                      JS_NewCFunction(ctx, js_rpc_listen, "listen", 1));

    JS_SetPropertyStr(ctx, rpc_obj, "sendTo",
                      JS_NewCFunction(ctx, js_rpc_sendTo, "sendTo", 3));

    // Добавляем метод on
    JS_SetPropertyStr(ctx, rpc_obj, "on_server",
                      JS_NewCFunction(ctx, js_rpc_on_server, "on_server", 2));
    JS_SetPropertyStr(ctx, rpc_obj, "on_client",
                      JS_NewCFunction(ctx, js_rpc_on_client, "on_client", 2));

    JS_SetPropertyStr(ctx, mtjs_obj, "rpc", rpc_obj);
}
