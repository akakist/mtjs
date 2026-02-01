#include <quickjs.h>
#include <unistd.h>
static JSValue js_sleep(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    if(argc!=1)
        return JS_ThrowSyntaxError(ctx, "must be 1 param - sleep timeout (sec)");
    if(!JS_IsNumber(argv[0]))
        return JS_ThrowSyntaxError(ctx, "sleep param must be a number");
    int32_t n;
    if(JS_ToInt32(ctx, &n,argv[0])<0)
        return JS_ThrowTypeError(ctx, "error to int");

    sleep(n);

    return JS_UNDEFINED;

}
static JSValue js_usleep(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    if(argc!=1)
        return JS_ThrowSyntaxError(ctx, "must be 1 param - usleep timeout (sec)");
    if(!JS_IsNumber(argv[0]))
        return JS_ThrowSyntaxError(ctx, "usleep param must be a number");
    int32_t n;
    if(JS_ToInt32(ctx, &n,argv[0])<0)
        return JS_ThrowTypeError(ctx, "error to int");

    usleep(n);

    return JS_UNDEFINED;

}

void init_utils_object(JSContext *ctx,JSValue & mtjs_obj)
{
    // Создаём объект rpc
    JSValue utils_obj = JS_NewObject(ctx);

    JS_SetPropertyStr(ctx, utils_obj, "sleep",
                      JS_NewCFunction(ctx, js_sleep, "sleep", 1));
    JS_SetPropertyStr(ctx, utils_obj, "usleep",
                      JS_NewCFunction(ctx, js_usleep, "usleep", 1));


    JS_SetPropertyStr(ctx, mtjs_obj, "utils", utils_obj);
}
