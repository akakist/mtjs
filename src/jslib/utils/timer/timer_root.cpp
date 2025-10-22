#include "quickjs.h"
#include "common/mtjs_opaque.h"
#include "Events/System/timerEvent.h"
#include "timeout.h"
#include "common/jsscope.h"
#include "common/timers.h"
#include "malloc_debug.h"

extern JSClassID js_timeout_class_id;

static uint64_t idGen=0;
static JSValue js_setCommon(JSContext *ctx, JSValueConst val, int argc, JSValueConst *argv, bool isInterval)
{
    mtjs_opaque *op = (mtjs_opaque *)JS_GetContextOpaque(ctx);
    JSScope <10,10> scope(ctx); 

    if (argc < 1) {
        logErr2("if(argc<1)");
        return JS_ThrowTypeError(ctx, "if(argc<1) js_setCommon");
    }

    JSValue cb = JS_DupValue(ctx, argv[0]);
    // scope.addValue(cb);
    if (!JS_IsFunction(ctx, cb)) {
        logErr2("if(!JS_IsFunction(ctx,cb))");
        return JS_EXCEPTION;
    }

    int64_t to;
    if (argc > 1) {
        if (JS_ToInt64(ctx, &to, argv[1]))
            return JS_EXCEPTION;
    }
    std::vector<JSValue> fargs;
    for (int i = 2; i < argc; i++) {
        JSValue arg = JS_DupValue(ctx, argv[i]);
        // scope.addValue(arg);
        fargs.push_back(arg);
        // scope.detach(arg); // Передаём владение аргументом в fargs
    }

    // uint64_t timerId = idGen++;
    REF_getter<TimerTask> t = new TimerTask;
    t->timerId=idGen++;
    t->ctx = ctx;
    t->isInterval = isInterval;
    t->fargs = std::move(fargs);
    t->timeout = double(to) / 1000.;
    t->cb = cb;
    for(auto& z: t->fargs)
    {
        JS_DupValue(ctx, z);

    }
    JS_DupValue(ctx, t->cb);


    // scope.detach(cb); // Передаём владение cb в t->cb
    op->rcf->timers.timers_refed.insert({t->timerId, t});

    JSValue jsTimeout = JS_NewObjectClass(ctx, js_timeout_class_id);

    // scope.addValue(jsTimeout);
    Timeout* timeout = new Timeout(t, op->rcf);

    JS_SetOpaque(jsTimeout, timeout);
    // scope.detach(jsTimeout); // Отсоединяем jsTimeout, так как оно возвращается

    if (isInterval) {
        logErr2("setTimer %lld %lf", t->timerId,t->timeout);
        op->broadcaster->sendEvent(ServiceEnum::Timer, new timerEvent::SetTimer(
                                       Timers::TIMER_INTERVAL, toRef(std::to_string(t->timerId)), t.get(), t->timeout, op->listener));
    } else {
        logErr2("SetAlarm %lld %lf", t->timerId,t->timeout);
        op->broadcaster->sendEvent(ServiceEnum::Timer, new timerEvent::SetAlarm(
                                       Timers::TIMER_TIMER, toRef(std::to_string(t->timerId)), t.get(), t->timeout, op->listener));
    }

    return jsTimeout;
}

static JSValue js_clearCommon(JSContext *ctx, JSValueConst val, int argc, JSValueConst *argv, bool isInterval)
{
    mtjs_opaque *op = (mtjs_opaque *)JS_GetContextOpaque(ctx);
    JSScope <10,10> scope(ctx); 

    if (argc < 1) {
        logErr2("if(argc<1)");
        return JS_ThrowTypeError(ctx, "if(argc<1) js_clearCommon");
    }
    // Извлечение Timeout из первого аргумента
    Timeout* timeoutObj = static_cast<Timeout*>(JS_GetOpaque(argv[0], js_timeout_class_id));
    if (!timeoutObj) {
        logErr2("Expected Timeout object as first argument");
        return JS_ThrowTypeError(ctx, "Expected Timeout object as first argument");
    }
    uint64_t timerId = timeoutObj->timerTask->timerId;

    if (isInterval) {
        logErr2("StopTimer %lld", timerId);
        op->broadcaster->sendEvent(ServiceEnum::Timer, new timerEvent::StopTimer(
                                       Timers::TIMER_INTERVAL, toRef(std::to_string(timerId)), op->listener));
    } else {
        logErr2("StopAlarm %lld", timerId);
        op->broadcaster->sendEvent(ServiceEnum::Timer, new timerEvent::StopAlarm(
                                       Timers::TIMER_TIMER, toRef(std::to_string(timerId)), op->listener));
    }
    op->rcf->timers.timers_refed.erase(timerId);
    op->rcf->timers.timers_unrefed.erase(timerId);

    return JS_UNDEFINED;
}

static JSValue js_setTimeout(JSContext *ctx, JSValueConst val, int argc, JSValueConst *argv)
{
    return js_setCommon(ctx,val,argc,argv,false);
}
static JSValue js_setInterval(JSContext *ctx, JSValueConst val, int argc, JSValueConst *argv)
{
    return js_setCommon(ctx,val,argc,argv,true);
}


static JSValue js_clearTimeout(JSContext *ctx, JSValueConst val, int argc, JSValueConst *argv)
{
    return js_clearCommon(ctx,val,argc,argv,false);
}
static JSValue js_clearInterval(JSContext *ctx, JSValueConst val, int argc, JSValueConst *argv)
{
    return js_clearCommon(ctx,val,argc,argv,true);
}

void register_timeout_class(JSContext* ctx);

void init_timer(JSRuntime *rt, JSContext *gCtx)
{
    JSValue gObj = JS_GetGlobalObject(gCtx);
    JS_DefinePropertyValueStr(gCtx, gObj, "setTimeout", JS_NewCFunction(gCtx, js_setTimeout, "setTimeout", 1), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(gCtx, gObj, "setInterval", JS_NewCFunction(gCtx, js_setInterval, "setInterval", 1), JS_PROP_C_W_E);

    JS_DefinePropertyValueStr(gCtx, gObj, "clearTimeout", JS_NewCFunction(gCtx, js_clearTimeout, "clearTimeout", 1), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(gCtx, gObj, "clearInterval", JS_NewCFunction(gCtx, js_clearInterval, "clearInterval", 1), JS_PROP_C_W_E);

    JS_FreeValue(gCtx, gObj);

    register_timeout_class(gCtx);

}
