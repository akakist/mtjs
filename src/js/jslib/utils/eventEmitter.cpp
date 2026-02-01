#include <quickjs.h>
#include <math.h>
#include "jsscope.h"
#include "eventEmitter.h"


#define countof(x) (sizeof(x) / sizeof((x)[0]))


typedef struct JSEventEmitterData_ {

    REF_getter<EventEmitter> emitter=nullptr;

} JSEventEmitterData;

static JSClassID js_event_emitter_class_id;

static void js_event_emitter_finalizer(JSRuntime *rt, JSValue val)
{
    JSEventEmitterData *s = (JSEventEmitterData *)JS_GetOpaque(val, js_event_emitter_class_id);
    /* Note: 's' can be NULL in case JS_SetOpaque() was not called */
    delete s;
}

static JSValue js_point_ctor(JSContext *ctx,
                             JSValueConst new_target,
                             int argc, JSValueConst *argv)
{
    //

    JSScope <10,10> scope(ctx);

    JSValue proto;
    proto = JS_GetPropertyStr(ctx, new_target, "prototype");
    scope.addValue(proto);
    if (JS_IsException(proto))
        return JS_EXCEPTION;
    // goto fail;
    JSValue obj = JS_UNDEFINED;
    obj = JS_NewObjectProtoClass(ctx, proto, js_event_emitter_class_id);
    if (JS_IsException(obj))
        return obj;
    JSEventEmitterData *s;
    s = new JSEventEmitterData;
    if (!s)
        return JS_EXCEPTION;
    JS_SetOpaque(obj, s);
    return obj;
}


JSValue new_event_emitter(JSContext *ctx,const REF_getter<EventEmitter>& emitter)
{
    JSEventEmitterData *s;
    JSValue obj = JS_NewObjectClass(ctx, js_event_emitter_class_id);
    if (JS_IsException(obj))
        return obj;
    s = new JSEventEmitterData;
    if (!s)
        return JS_EXCEPTION;
    s->emitter = emitter;
    JS_SetOpaque(obj, s);
    return obj;
}
static JSValue js_on(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    JSEventEmitterData *s = (JSEventEmitterData *)JS_GetOpaque2(ctx, this_val, js_event_emitter_class_id);
    if (!s)
        return JS_EXCEPTION;
    JSScope<10,10>  scope(ctx);
    if (argc < 2) return JS_ThrowTypeError(ctx, "on(eventName, ...callback)");
    if (!JS_IsString(argv[0]))
        return JS_ThrowTypeError(ctx, "eventName must be a string");

    auto eventName=scope.toStdStringView(argv[0]);
    if (eventName.empty()) return JS_ThrowTypeError(ctx, "eventName must not be empty");

    for(int i=1; i<argc; i++)
    {
        if (!JS_IsFunction(ctx, argv[i]))
            return JS_ThrowTypeError(ctx, "callback must be a function");
        s->emitter->on(std::string(eventName), argv[i]);
    }

    return JS_DupValue(ctx, this_val);
}


static JSValue js_emit(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    JSEventEmitterData *s = (JSEventEmitterData *)JS_GetOpaque2(ctx, this_val, js_event_emitter_class_id);
    if (!s)
        return JS_ThrowInternalError(ctx, "emit: if(!s)");
    JSScope <10,10> scope(ctx);
    if (argc < 2) return JS_ThrowTypeError(ctx, "emit(eventName, ...args)");
    if (!JS_IsString(argv[0])) return JS_ThrowTypeError(ctx, "eventName must be a string");
    auto eventName=scope.toStdStringView(argv[0]);
    if (eventName.empty()) return JS_ThrowTypeError(ctx, "eventName must not be empty");

    int cnt=0;
    cnt+=s->emitter->emit(std::string(eventName),argc-1, &argv[1]);

    return JS_NewBool(ctx, cnt>0);
}
static JSValue vectorToJSArray(JSContext *ctx, const std::vector<JSValue> &vec) {
    JSValue jsArray = JS_NewArray(ctx);  // Создаём новый JS-массив

    for (size_t i = 0; i < vec.size(); ++i) {
        // Устанавливаем элемент в массив (индекс, значение)
        JS_SetPropertyUint32(ctx, jsArray, i, JS_DupValue(ctx, vec[i]));
    }

    return jsArray;  // Возвращаем JS-массив
}
static JSValue js_get_listeners(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    JSEventEmitterData *s = (JSEventEmitterData *)JS_GetOpaque2(ctx, this_val, js_event_emitter_class_id);
    if (!s)
        return JS_ThrowInternalError(ctx, "js_get_listeners: if(!s)");
    JSScope <10,10> scope(ctx);
    if (argc != 1) return JS_ThrowTypeError(ctx, "js_get_listeners argc must be 1");

    if (!JS_IsString(argv[0])) return JS_ThrowTypeError(ctx, "eventName must be a string");
    auto eventName=scope.toStdStringView(argv[0]);
    if (eventName.empty()) return JS_ThrowTypeError(ctx, "eventName must not be empty");

    auto listeners=s->emitter->listeners(std::string(eventName));

    JSValue arr = vectorToJSArray(ctx, listeners);

    return arr;

}

static JSClassDef js_point_class = {
    "EventEmitter",
    .finalizer = js_event_emitter_finalizer,
};

static const JSCFunctionListEntry js_point_proto_funcs[] = {
    JS_CFUNC_DEF("on", 2, js_on),
    JS_CFUNC_DEF("emit", 2, js_emit),
};

static int js_event_listener_init(JSContext *ctx, JSModuleDef *m)
{
    JSValue event_emitter_proto, event_emitter_class;

    JS_NewClassID(&js_event_emitter_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_event_emitter_class_id, &js_point_class);

    event_emitter_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, event_emitter_proto, js_point_proto_funcs, countof(js_point_proto_funcs));

    event_emitter_class = JS_NewCFunction2(ctx, js_point_ctor, "EventEmitter", 2, JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */
    JS_SetConstructor(ctx, event_emitter_class, event_emitter_proto);
    JS_SetClassProto(ctx, js_event_emitter_class_id, event_emitter_proto);

    JS_SetModuleExport(ctx, m, "EventEmitter", event_emitter_class);
    return 0;
}

JSModuleDef *js_init_module_event_emitter(JSContext *ctx, const char *module_name)
{
    JSModuleDef *m;
    m = JS_NewCModule(ctx, module_name, js_event_listener_init);
    if (!m)
        return NULL;
    JS_AddModuleExport(ctx, m, "EventEmitter");
    return m;
}
