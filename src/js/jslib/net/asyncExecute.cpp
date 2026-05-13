#include <exception>
#include <quickjs.h>
#include <stdlib.h>
#include <string>
#include "REF.h"
#include "common/async_task.h"
#include "common/jsscope.h"
#include "common/mtjs_opaque.h"
#include "commonError.h"
#include "jHolder.h"
#include "listenerBase.h"
#include "mutexInspector.h"

static JSClassID js_curl_response_class_id;

struct execute_task : public async_task
{
    execute_task(ListenerBase *l) : async_task(l)
    {
    }
    ~execute_task()
    {
    }

    void execute();

    void finalize(JSContext *ctx);

    // JSValue promise_data[2];
    JHolder resolve;
    JHolder reject;
    // JVAlue

    int rv = 0;
    std::string error_str;

    std::string cmd;
};

JSValue js_execute_async(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    MUTEX_INSPECTOR;

    try
    {
        mtjs_opaque *op = (mtjs_opaque *)JS_GetContextOpaque(ctx);

        if (argc != 1)
            return JS_ThrowInternalError(ctx, "number of argument must be 2");

        if (!JS_IsString(argv[0]))
            return JS_ThrowTypeError(ctx, "if(!JS_IsString(ctx,argv[0]))");

        JSScope<10, 10> scope(ctx);

        REF_getter<execute_task> task = new execute_task(op->listener_);

        JSValue _promise[2];
        JSValue promise = JS_NewPromiseCapability(ctx, _promise);
        JHolder resolve(ctx,_promise[0]);
        JHolder reject(ctx,_promise[1]);
        JS_FreeValue(ctx,_promise[0]);
        JS_FreeValue(ctx,_promise[1]);
        // task->promise_data
        // JSValue promise;
        if (JS_IsException(promise))
        {
            JS_FreeValue(ctx, _promise[0]);
            JS_FreeValue(ctx, _promise[1]);
            return JS_ThrowInternalError(ctx, "JS_NewPromiseCapability error");
        }

        task->cmd = scope.toStdStringView(argv[0]); // JS_ToCString(ctx, argv[0]);

        task->promise_data[0] = JS_DupValue(ctx, _promise[0]);
        task->promise_data[1] = JS_DupValue(ctx, _promise[1]);
        op->async_deque->push(task.get());

        JS_FreeValue(ctx, _promise[0]);
        JS_FreeValue(ctx, _promise[1]);
        return promise;
    }
    catch (std::exception &e)
    {
        logErr2("exception %s", e.what());
        return JS_ThrowInternalError(ctx, "exception %s", e.what());
    }
    return JS_ThrowInternalError(ctx, "return values wrong");
}
void execute_task::execute()
{
    MUTEX_INSPECTOR;
    rv = system(cmd.c_str());
    if (rv != 0)
    {
        error_str = std::string("execute command '") + cmd + "' failed with code " + std::to_string(rv);
        logErr2("%s", error_str.c_str());
    }
}

void execute_task::finalize(JSContext *ctx)
{
    MUTEX_INSPECTOR;
    JSScope<10, 10> scope(ctx);
    if (rv == 0)
    {
        auto ret = JS_Call(ctx, resolve.listener, JS_UNDEFINED, 0, nullptr);
        scope.addValue(ret);
    }
    else
    {
        JSValue str = JS_NewString(ctx, error_str.c_str());

        scope.addValue(str);
        auto ret = JS_Call(ctx, reject.listener, JS_UNDEFINED, 1, &str);
        scope.addValue(ret);
    }
    // qjs::free_promise_callbacks(ctx, promise_data);
}

void js_register_async_tools(JSContext *ctx, JSValue &mtjs_obj)
{
    MUTEX_INSPECTOR;
    JS_SetPropertyStr(ctx, mtjs_obj, "asyncExecute", JS_NewCFunction(ctx, js_execute_async, "asyncExecute", 2));
}
