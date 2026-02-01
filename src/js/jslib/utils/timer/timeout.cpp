#include <quickjs.h>
#include <string>
#include "timeout.h"

JSClassID js_timeout_class_id;

static void js_response_finalizer(JSRuntime* rt, JSValue val) {
    Timeout* req = static_cast<Timeout*>(JS_GetOpaque(val, js_timeout_class_id));
    if (req) {
        delete req;
    }
}

static JSClassDef js_timeout_class = {
    "Response",
    .finalizer = js_response_finalizer,
};

static JSValue js_toString(JSContext *ctx, JSValueConst this_val,
                           int argc, JSValueConst *argv)
{
    Timeout* req = static_cast<Timeout*>(JS_GetOpaque2(ctx, this_val, js_timeout_class_id));

    // Добавленная проверка
    if (!req || !req->timerTask.valid()) {
        return JS_ThrowReferenceError(ctx, "Timeout object or timerTask is null");
    }

    std::string str = "Timeout: " + std::to_string(req->timerTask->timerId);
    JSValue ret = JS_NewString(ctx, str.c_str());
    return ret;
}

static JSValue js_unref(JSContext *ctx, JSValueConst this_val,
                        int argc, JSValueConst *argv)
{
    logErr2("js unref");
    Timeout* req = static_cast<Timeout*>(JS_GetOpaque2(ctx, this_val, js_timeout_class_id));

    // Добавленная проверка
    if (!req || !req->timerTask.valid()) {
        return JS_ThrowReferenceError(ctx, "Timeout object or timerTask is null");
    }

    logErr2("timer@@Id %lld", req->timerTask->timerId);
    auto it = req->rcf->timers.timers_refed.find(req->timerTask->timerId);
    if (it != req->rcf->timers.timers_refed.end())
    {
        req->rcf->timers.timers_unrefed.insert({req->timerTask->timerId, it->second});
        req->rcf->timers.timers_refed.erase(it);
    }
    else
    {
        return JS_ThrowReferenceError(ctx, "Не могу найти таймер в refed");
    }

    return JS_DupValue(ctx, this_val);
}

static const JSCFunctionListEntry js_timeout_proto_funcs[] = {
    JS_CFUNC_DEF("unref", 0, js_unref),
    JS_CFUNC_DEF("toString", 0, js_toString),
};

void register_timeout_class(JSContext* ctx)
{
    JS_NewClassID(&js_timeout_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_timeout_class_id, &js_timeout_class);

    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_timeout_proto_funcs, sizeof(js_timeout_proto_funcs) / sizeof(JSCFunctionListEntry));

    JS_SetClassProto(ctx, js_timeout_class_id, proto);
}
