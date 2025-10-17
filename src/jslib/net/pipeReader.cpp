#include "quickjs.h"
#include "common/jsscope.h"
#include "common/mtjs_opaque.h"
#include <iostream>
#include "malloc_debug.h"
#include "stream.h"
#include "ioStreams.h"
#include <iostream>
#include "async_task.h"


struct c_pipe_async_reader_buf: public async_task
{
    std::string path;
    REF_getter<Stream> stream=nullptr;
    int rv=-1;
    std::string errstr;
    JSValue promise_data[2];

    c_pipe_async_reader_buf(ListenerBase* l)
        :async_task(l)
    {
    }

    void execute() final
    {
        FILE *pipe=popen(path.c_str(),"r");
        if(!pipe)
        {
            stream->write("error", "popen failed "+path);
            errstr= (std::string)"popen failed "+path+" "+ strerror(errno);
            rv=-1;
            return;
        }

        char buf[1024*64];
        int r=0;
        while ((r = fread(buf, 1, sizeof(buf), pipe)) > 0)
        {
            std::string ret(buf, r);
            stream->write("data", ret);
        }
        stream->write("end", "");
        fclose(pipe);
        rv=0;
    }

    void finalize(JSContext *ctx)
    {
        JSScope scope(ctx);
        if(rv<0)
        {
            JSValue r=JS_NewError(ctx);
            DBG(memctl_add_object(r, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

            scope.addValue(r);
            JS_DefinePropertyValueStr(ctx, r, "message",
                                      JS_NewString(ctx, errstr.c_str()),
                                      JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
            auto ret=JS_Call(ctx, promise_data[1], JS_UNDEFINED, 1, &r);
            scope.addValue(ret);

        }
        else
        {
            JSValue z=JS_UNDEFINED;
            auto res=JS_Call(ctx, promise_data[0], JS_UNDEFINED, 1, &z);
            scope.addValue(res);
        }
        qjs::free_promise_callbacks(ctx,promise_data);

    }

    ~c_pipe_async_reader_buf()
    {
    }
};

static JSValue js_read_pipe_buf(JSContext *ctx, JSValueConst this_val,
                                int argc, JSValueConst *argv)
{
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);

    JSScope scope(ctx);
    if (argc < 2 || !JS_IsString(argv[0]) || !js_IsStream(ctx, argv[1])) {
        return JS_ThrowTypeError(ctx, "Expected (string path, stream)");
    }

    std::string _path = scope.toStdString(argv[0]);

    auto s=js_get_stream_CPP(ctx, argv[1]);
    if(!s.valid())
    {
        return JS_ThrowTypeError(ctx, "Expected (string path, stream)");
    }

    REF_getter<c_pipe_async_reader_buf> f=new c_pipe_async_reader_buf(op->listener);
    f->path=_path;
    f->stream=s;
    JSValue promise = JS_NewPromiseCapability(ctx, f->promise_data);
    DBG(memctl_add_object(promise, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

    if (JS_IsException(promise)) {
        return JS_ThrowInternalError(ctx,"JS_NewPromiseCapability error");
    }

    op->async_deque->push(f.get());
    return promise;

}
struct c_pipe_async_reader_lines: public async_task
{
    std::string path;
    REF_getter<Stream> stream=nullptr;
    int rv=-1;
    std::string errstr;
    JSValue promise_data[2];

    c_pipe_async_reader_lines(ListenerBase* l)
        :async_task(l)
    {
    }

    void execute() final
    {
        FILE *pipe=popen(path.c_str(),"r");
        if(!pipe)
        {
            stream->write("error", "popen failed "+path);
            errstr= (std::string)"popen failed "+path+" "+ strerror(errno);
            rv=-1;
            return;
        }

        char *line = nullptr;
        size_t len = 0;

        while (getline(&line, &len, pipe) != -1) {
            stream->write("data", line);
        }
        stream->write("end", "");

        free(line);
        fclose(pipe);


        // char buf[1024*64];
        // int r=0;
        // while ((r = fread(buf, 1, sizeof(buf), pipe)) > 0)
        // {
        //     std::string ret(buf, r);
        //     stream->write(PKT_DATA, ret);
        // }
        // stream->write(PKT_END, "");
        // fclose(pipe);
        rv=0;
    }

    void finalize(JSContext *ctx)
    {
        JSScope scope(ctx);
        if(rv<0)
        {
            JSValue r=JS_NewError(ctx);
            DBG(memctl_add_object(r, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

            scope.addValue(r);
            JS_DefinePropertyValueStr(ctx, r, "message",
                                      JS_NewString(ctx, errstr.c_str()),
                                      JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
            auto ret=JS_Call(ctx, promise_data[1], JS_UNDEFINED, 1, &r);
            scope.addValue(ret);

        }
        else
        {
            JSValue z=JS_UNDEFINED;
            auto res=JS_Call(ctx, promise_data[0], JS_UNDEFINED, 1, &z);
            scope.addValue(res);
        }
        qjs::free_promise_callbacks(ctx,promise_data);

    }

    ~c_pipe_async_reader_lines()
    {
    }
};

static JSValue js_read_pipe_lines(JSContext *ctx, JSValueConst this_val,
                                  int argc, JSValueConst *argv)
{
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);

    JSScope scope(ctx);
    if (argc < 2 || !JS_IsString(argv[0]) || !js_IsStream(ctx, argv[1])) {
        return JS_ThrowTypeError(ctx, "Expected (string path, stream)");
    }

    auto _path =scope.toStdString(argv[0]);
    auto stream=js_get_stream_CPP(ctx, argv[1]);
    if(!stream.valid())
    {
        return JS_ThrowTypeError(ctx, "Expected (string path, stream)");
    }

    REF_getter<c_pipe_async_reader_lines> f=new c_pipe_async_reader_lines(op->listener);
    f->path=_path;
    f->stream=stream;
    JSValue promise = JS_NewPromiseCapability(ctx, f->promise_data);
    DBG(memctl_add_object(promise, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

    if (JS_IsException(promise)) {
        return JS_ThrowInternalError(ctx,"JS_NewPromiseCapability error");
    }

    op->async_deque->push(f.get());
    return promise;



}

struct c_stdin_async_reader_lines: public async_task
{
    REF_getter<Stream> stream=nullptr;
    int rv=-1;
    std::string errstr;
    JSValue promise_data[2];

    c_stdin_async_reader_lines(ListenerBase* l)
        :async_task(l)
    {
    }

    void execute() final
    {

        std::string line;
        logErr2("begin getlines");
        while (std::getline(std::cin, line)) {
            stream->write("data", line);
        }
        stream->write("end", "");

        rv=0;
    }

    void finalize(JSContext *ctx)
    {
        JSScope scope(ctx);
        if(rv<0)
        {
            JSValue r=JS_NewError(ctx);
            DBG(memctl_add_object(r, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

            scope.addValue(r);
            JS_DefinePropertyValueStr(ctx, r, "message",
                                      JS_NewString(ctx, errstr.c_str()),
                                      JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
            auto ret=JS_Call(ctx, promise_data[1], JS_UNDEFINED, 1, &r);
            scope.addValue(ret);

        }
        else
        {
            JSValue z=JS_UNDEFINED;
            auto res=JS_Call(ctx, promise_data[0], JS_UNDEFINED, 1, &z);
            scope.addValue(res);
        }
        qjs::free_promise_callbacks(ctx,promise_data);

    }

    ~c_stdin_async_reader_lines()
    {
    }
};

static JSValue js_read_stdin_lines(JSContext *ctx, JSValueConst this_val,
                                   int argc, JSValueConst *argv)
{
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);

    JSScope scope(ctx);
    if (argc < 1  || !js_IsStream(ctx, argv[0])) {
        return JS_ThrowTypeError(ctx, "Expected stream");
    }

    auto stream=js_get_stream_CPP(ctx, argv[0]);
    if(!stream.valid())
    {
        return JS_ThrowTypeError(ctx, "Expected stream");
    }
    REF_getter<c_stdin_async_reader_lines> task=new c_stdin_async_reader_lines(op->listener);
    task->stream=stream;
    JSValue promise = JS_NewPromiseCapability(ctx, task->promise_data);
    DBG(memctl_add_object(promise, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

    if (JS_IsException(promise)) {
        return JS_ThrowInternalError(ctx,"JS_NewPromiseCapability error");
    }

    op->async_deque->push(task.get());
    return promise;


}
void js_register_pipe_reader( JSContext *ctx,JSValue & mtjs_obj)
{
    JSValue pipeObj = JS_NewObject(ctx);
    DBG(memctl_add_object(pipeObj, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

    JS_SetPropertyStr(ctx, pipeObj, "read_lines", JS_NewCFunction(ctx, js_read_pipe_lines, "read_lines", 2));
    JS_SetPropertyStr(ctx, pipeObj, "read", JS_NewCFunction(ctx, js_read_pipe_buf, "read", 2));
    JS_SetPropertyStr(ctx, mtjs_obj, "pipe", pipeObj);

}

int js_init_stdin_reader_global(JSContext *ctx,JSValue & mtjs_obj)
{

    JSValue stdinObj = JS_NewObject(ctx);
    DBG(memctl_add_object(stdinObj, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));


    JSValue js_read_stdin_lines_f = JS_NewCFunction(ctx, js_read_stdin_lines, "read_lines", 1);
    DBG(memctl_add_object(js_read_stdin_lines_f, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

    JS_SetPropertyStr(ctx, stdinObj, "read_lines", js_read_stdin_lines_f);



    JS_SetPropertyStr(ctx, mtjs_obj, "STDIN", stdinObj);


    return 0; // успех
}

