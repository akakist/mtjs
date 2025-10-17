#include <quickjs.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "common/async_task.h"
#include "common/jsscope.h"
#include "common/mtjs_opaque.h"

#include "malloc_debug.h"

static JSClassID js_curl_response_class_id;

struct execute_task: public async_task
{
    execute_task(ListenerBase* l): async_task(l) {
        DBG(iUtils->mem_add_ptr("execute_task",this));

    }
    ~execute_task()
    {
        DBG(iUtils->mem_remove_ptr("execute_task",this));
    }

    void execute();

    void finalize(JSContext* ctx);

    JSValue promise_data[2];

    int rv=0;
    std::string error_str;

    std::string cmd;
};


JSValue js_execute_async(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    MUTEX_INSPECTOR;


    try {
        mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);

        if(argc !=1)return JS_ThrowInternalError(ctx, "number of argument must be 2");

        if(!JS_IsString(argv[0]))
            return JS_ThrowTypeError(ctx, "if(!JS_IsString(ctx,argv[0]))");

        JSScope scope(ctx);

        REF_getter<execute_task> task=new execute_task(op->listener);

        JSValue promise = JS_NewPromiseCapability(ctx, task->promise_data);
        DBG(memctl_add_object(promise, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

        if (JS_IsException(promise)) {
            return JS_ThrowInternalError(ctx,"JS_NewPromiseCapability error");
        }

        task->cmd = scope.toStdString(argv[0]);// JS_ToCString(ctx, argv[0]);

        op->async_deque->push(task.get());

        return promise;
    } catch(std::exception &e)
    {
        logErr2("exception %s",e.what());
        return JS_ThrowInternalError(ctx, "exception %s",e.what());
    }
    return JS_ThrowInternalError(ctx, "return values wrong");
}
void execute_task::execute()
{
    MUTEX_INSPECTOR;
    rv=system(cmd.c_str());
    if(rv!=0)
    {
        error_str=std::string("execute command '")+cmd+"' failed with code "+std::to_string(rv);
        logErr2("%s",error_str.c_str());
    }

}

void execute_task::finalize(JSContext* ctx)
{
    MUTEX_INSPECTOR;
    JSScope scope(ctx);
    if(rv==0)
    {
        auto ret=JS_Call(ctx, promise_data[0], JS_UNDEFINED, 0, nullptr);
        scope.addValue(ret);
    } else {
        JSValue str=JS_NewString(ctx, error_str.c_str());
        DBG(memctl_add_object(str, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

        scope.addValue(str);
        auto ret=JS_Call(ctx, promise_data[1], JS_UNDEFINED, 1, &str);
        scope.addValue(ret);
    }
    qjs::free_promise_callbacks(ctx,promise_data);


}


void js_register_async_tools( JSContext *ctx,JSValue & mtjs_obj)
{
    MUTEX_INSPECTOR;
    JS_SetPropertyStr(ctx, mtjs_obj, "asyncExecute", JS_NewCFunction(ctx, js_execute_async, "asyncExecute", 2));

}
