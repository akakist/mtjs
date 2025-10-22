#include "quickjs.h"
#include <math.h>
#include "common/ioStreams.h"
#include "common/jsscope.h"
#include "eventEmitter.h"
#include "malloc_debug.h"
#include "listenerBase.h"
#include "mtjsEvent.h"
#include "mtjs_opaque.h"

struct ConstReadableStringStream: public Stream
{
    std::string buf;
    std::string read(size_t n) final
    {
        if(n>=buf.size())
        {
            auto s=std::move(buf);
            buf.clear();
            return s;
        }
        else if(buf.size()==0)
            return "";
        else
        {
            auto s=buf.substr(0,n);
            buf=buf.substr(n,buf.size()-n);
            return s;
        }
    }
    bool rd_hasContentLength() final
    {
        return true;
    }
    size_t rd_contentLength() final
    {
        return buf.size();
    }
    void write(const std::string& cmd, const char* buf, size_t sz) final
    {
        throw CommonError("cannot write to string buffer");
    }

    ConstReadableStringStream(const std::string &s)
        :Stream("ConstReadableStringStream"), buf(s)
    {
        DBG(iUtils->mem_add_ptr("ConstReadableStringStream",this));

    }
    ~ConstReadableStringStream()
    {
        DBG(iUtils->mem_remove_ptr("ConstReadableStringStream",this));
    }

};
struct WriteableStringStream: public Stream
{
    std::deque<std::string> buf;
    MutexC mutex;
    Condition cond;
    bool streamEnded=false;
    WriteableStringStream():Stream("WriteableStringStream"), cond(mutex) {

        DBG(iUtils->mem_add_ptr("WriteableStringStream",this));
    }
    ~WriteableStringStream()
    {
        DBG(iUtils->mem_remove_ptr("WriteableStringStream",this));
    }
    void write(const std::string& cmd, const char *xbuf, size_t sz) final
    {
        if(piped.valid())
        {
            piped->write(cmd,xbuf,sz);
            return;
        }
        {
            M_LOCKC(mutex);
            if(cmd=="data")
                buf.push_back(std::string(xbuf,sz));
            else if(cmd=="end")
            {
                buf.push_back(std::string(xbuf,sz));
                streamEnded=true;
            }
            else {
                logErr2("unhandled command %s in WriteableStringStream",cmd.c_str());
            }
        }
        cond.signal();
    }
    bool rd_hasContentLength() final
    {
        return false;
    }
    size_t rd_contentLength() final
    {
        return 0;
    }
    std::string read(size_t n) final
    {
        if(piped.valid())
            throw CommonError("attempt read from piped stream");

        M_LOCKC(mutex);
        while(1)
        {
            if(buf.size())
            {
                if(buf[0].size()<=n)
                {
                    auto ss=std::move(buf[0]);
                    buf.pop_front();
                    return ss;
                }
                else {

                    auto s=buf[0].substr(0,n);
                    buf[0]=buf[0].substr(n);
                    return s;

                }
            }
            if(buf.size()==0 && streamEnded) return "";

            cond.wait();
        }
    }

};

struct FileReadableStream: public Stream
{
    FILE *fp;
    size_t content_length=0;
    std::string filename;
    FileReadableStream(const std::string &fn)
        :Stream("FileReadableStream"), filename(fn)
    {
        fp=fopen(fn.c_str(),"rb");
        if(!fp)
        {
            throw std::runtime_error("FileReadableStream: cannot open file "+fn+" "+strerror(errno));
        }
        DBG(iUtils->mem_add_ptr("FileReadableStream",this));

    }
    ~FileReadableStream()
    {
        fclose(fp);
        DBG(iUtils->mem_remove_ptr("FileReadableStream",this));
    }
    std::string read(size_t n) final
    {
        char buf[63*1024];
        size_t r=fread(buf,1,sizeof(buf),fp);
        if(r>0)
        {
            return std::string(buf,r);
        }
        else
        {
            return "";
        }
    }
    bool rd_hasContentLength() final
    {
        return false;
    }
    size_t rd_contentLength() final
    {
        throw CommonError("FileReadableStream: content length not available");
    }
    void write(const std::string& cmd, const char *, size_t) final
    {
        throw CommonError("invalid call %s ",__func__);
    }
};
struct FileWriteableStream: public Stream
{
    FILE *fp;
    FileWriteableStream(const std::string &fn): Stream("FileWriteableStream")
    {
        fp=fopen(fn.c_str(),"wb");
        if(!fp)
        {
            throw std::runtime_error("FileWriteableStream: cannot open file "+fn);
        }
        DBG(iUtils->mem_add_ptr("FileWriteableStream",this));

    }
    ~FileWriteableStream()
    {
        if(fp)
            fclose(fp);

        DBG(iUtils->mem_remove_ptr("FileWriteableStream",this));
    }
    void write(const std::string& cmd, const char* buf, size_t sz) final
    {
        if(cmd=="data")
        {
            fwrite(buf,1,sz,fp);
        }
        else if(cmd=="end")
        {
            fclose(fp);
            fp=nullptr;
        }
        else
        {
            logErr2("unhandled command %s in FileWriteableStream",cmd.c_str());
        }
    }
    std::string read(size_t n) final
    {
        throw CommonError("unimpl %s",__func__);
    }
    bool rd_hasContentLength() final
    {
        return false;
    }
    size_t rd_contentLength() final
    {
        throw CommonError("FileReadableStream: content length not available");
    }

};
struct EventEmitterStream: public Stream
{
    REF_getter<EventEmitter> emitter;
    std::deque<std::string> buf;
    bool streamEnded=false;
    ListenerBase *listener=nullptr;
    EventEmitterStream(JSContext* ctx, ListenerBase* l):Stream("EventEmitterStream"), emitter(new EventEmitter(ctx,"EventEmitterStream")), listener(l)
    {

        DBG(iUtils->mem_add_ptr("EventEmitterStream",this));
    }
    ~EventEmitterStream()
    {
        DBG(iUtils->mem_remove_ptr("EventEmitterStream",this));
    }
    void write(const std::string& cmd, const char *buf, size_t sz) final
    {

        listener->listenToEvent(new mtjsEvent::EmitterData(cmd,std::string(buf,sz),emitter));

    }
    bool rd_hasContentLength() final
    {
        return false;
    }
    size_t rd_contentLength() final
    {
        return 0;
    }
    std::string read(size_t n) final
    {
        throw CommonError("call obsolete read %d",__LINE__);
    }

};


#define countof(x) (sizeof(x) / sizeof((x)[0]))

/* Stream Class */

struct JSStreamData {
    REF_getter<Stream> stream=nullptr;

    JSStreamData()
    {
        DBG(iUtils->mem_add_ptr("JSStreamData",this));

    }
    ~JSStreamData()
    {
        DBG(iUtils->mem_remove_ptr("JSStreamData",this));
    }
};

static JSClassID js_stream_class_id;
// static JSClassID js_ConstReadableStringStream_class_id;


static void js_stream_finalizer(JSRuntime *rt, JSValue val)
{
    JSStreamData *s = (JSStreamData *)JS_GetOpaque(val, js_stream_class_id);
    delete s;
    /* Note: 's' can be NULL in case JS_SetOpaque() was not called */
}



static JSValue js_WriteableStringStream_ctor(JSContext *ctx,
        JSValueConst new_target,
        int argc, JSValueConst *argv)
{
    JSValue obj = JS_UNDEFINED;

    JSScope <10,10> scope(ctx);
    if(argc!=0) return JS_ThrowSyntaxError(ctx, "no params required");

    JSStreamData *s;
    s = new JSStreamData;
    if (!s)
        return JS_EXCEPTION;

    s->stream=new WriteableStringStream();
    JSValue proto = JS_GetPropertyStr(ctx, new_target, "prototype");
    if (JS_IsException(proto))
        return JS_EXCEPTION;
    scope.addValue(proto);
    obj = JS_NewObjectProtoClass(ctx, proto, js_stream_class_id);
    if (JS_IsException(obj))
        return JS_EXCEPTION;
    JS_SetOpaque(obj, s);
    return obj;
}

static JSValue js_ConstReadableStringStream_ctor(JSContext *ctx,
        JSValueConst new_target,
        int argc, JSValueConst *argv)
{
    JSStreamData *s;
    JSValue obj = JS_UNDEFINED;

    JSScope <10,10> scope(ctx);

    std::string str;
    for(int i=0; i<argc; i++)
    {
        if(!JS_IsString(argv[i])) return JS_ThrowSyntaxError(ctx,"param i must be string");
        str+=scope.toStdStringView(argv[i]);


    }

    s = new JSStreamData;
    if (!s)
        return JS_EXCEPTION;

    s->stream=new ConstReadableStringStream(str);
    JSValue proto = JS_GetPropertyStr(ctx, new_target, "prototype");
    scope.addValue(proto);
    if (JS_IsException(proto))
        return JS_EXCEPTION;
    obj = JS_NewObjectProtoClass(ctx, proto, js_stream_class_id);

    // scope.addValue(obj);
    if (JS_IsException(obj))
        return JS_EXCEPTION;
    JS_SetOpaque(obj, s);
    return obj;
}
static JSValue js_FileReadableStream_ctor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv)
{
    JSStreamData *s;
    JSValue obj = JS_UNDEFINED;

    JSScope <10,10> scope(ctx);

    if(argc!=1)
        return JS_ThrowSyntaxError(ctx,"ctor 1 param - filename");

    std::string str;

    if(!JS_IsString(argv[0])) return JS_ThrowSyntaxError(ctx,"param i must be string");
    str=scope.toStdStringView(argv[0]);

    s = new JSStreamData;
    if (!s)
        return JS_EXCEPTION;

    s->stream=new FileReadableStream(str);
    JSValue proto = JS_GetPropertyStr(ctx, new_target, "prototype");
    scope.addValue(proto);
    if (JS_IsException(proto))
        return JS_EXCEPTION;
    obj = JS_NewObjectProtoClass(ctx, proto, js_stream_class_id);

    // scope.addValue(obj);
    if (JS_IsException(obj))
        return JS_EXCEPTION;
    JS_SetOpaque(obj, s);
    return obj;
}
static JSValue js_FileWriteableStream_ctor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv)
{

    JSScope <10,10> scope(ctx);

    if(argc!=1)
        return JS_ThrowSyntaxError(ctx,"ctor 1 param - filename");

    std::string str;

    if(!JS_IsString(argv[0])) return JS_ThrowSyntaxError(ctx,"param i must be string");
    str=scope.toStdStringView(argv[0]);

    JSStreamData *s;
    s = new JSStreamData;
    if (!s)
        return JS_EXCEPTION;

    s->stream=new FileWriteableStream(str);
    JSValue proto = JS_GetPropertyStr(ctx, new_target, "prototype");
    scope.addValue(proto);
    if (JS_IsException(proto))
        return JS_EXCEPTION;
    JSValue obj = JS_NewObjectProtoClass(ctx, proto, js_stream_class_id);

    // scope.addValue(obj);
    if (JS_IsException(obj))
        return JS_EXCEPTION;
    JS_SetOpaque(obj, s);
    return obj;
}
static JSValue js_EventEmitterStream_ctor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv)
{
    mtjs_opaque *op=(mtjs_opaque *) JS_GetContextOpaque(ctx);
    if(!op) return JS_ThrowSyntaxError(ctx,"EventEmitterStream: no mtjs_opaque");

    JSScope <10,10> scope(ctx);

    if(argc!=0)
        return JS_ThrowSyntaxError(ctx,"ctor 0 param ");

    std::string str;

    JSStreamData *s;
    s = new JSStreamData;
    if (!s)
        return JS_EXCEPTION;

    s->stream=new EventEmitterStream(ctx,op->listener);
    JSValue proto = JS_GetPropertyStr(ctx, new_target, "prototype");
    scope.addValue(proto);
    if (JS_IsException(proto))
        return JS_EXCEPTION;
    JSValue obj = JS_NewObjectProtoClass(ctx, proto, js_stream_class_id);

    // scope.addValue(obj);
    if (JS_IsException(obj))
        return JS_EXCEPTION;
    JS_SetOpaque(obj, s);
    return obj;
}


static JSValue js_stream_write(JSContext *ctx, JSValueConst this_val,
                               int argc, JSValueConst *argv)
{
    JSScope <10,10> scope(ctx);
    JSStreamData *s = (JSStreamData *) JS_GetOpaque2(ctx, this_val, js_stream_class_id);
    if (!s) return JS_EXCEPTION;
    for(int i=0; i<argc; i++)
    {
        if(!JS_IsString(argv[i])) return JS_ThrowSyntaxError(ctx, "param must be string");
        auto str=scope.toStdStringView(argv[i]);
        s->stream->write("data",str.data(),str.size());
    }

    return JS_UNDEFINED;
}
static JSValue js_stream_on(JSContext *ctx, JSValueConst this_val,
                            int argc, JSValueConst *argv)
{
    JSScope <10,10> scope(ctx);
    JSStreamData *s = (JSStreamData *) JS_GetOpaque2(ctx, this_val, js_stream_class_id);
    if (!s) return JS_EXCEPTION;
    if(argc!=2) return JS_ThrowSyntaxError(ctx, "on event name and callback required");
    if(!JS_IsString(argv[0])) return JS_ThrowSyntaxError(ctx, "event name must be string");
    if(!JS_IsFunction(ctx, argv[1])) return JS_ThrowSyntaxError(ctx, "callback must be function");
    auto eventName=scope.toStdStringView(argv[0]);
    JSValue callback=argv[1];
    EventEmitterStream *emitterStream=dynamic_cast<EventEmitterStream *>(s->stream.get());
    if(!emitterStream)
    {
        return JS_ThrowTypeError(ctx, "Stream is not EventEmitterStream");
    }
    if(!emitterStream->emitter.valid())
    {
        return JS_ThrowTypeError(ctx, "EventEmitterStream has no emitter");
    }
    if(!emitterStream->listener)
    {
        return JS_ThrowTypeError(ctx, "EventEmitterStream has no listener");
    }
    emitterStream->emitter->on(std::string(eventName),callback);

    return JS_UNDEFINED;
}

static JSValue js_stream_end(JSContext *ctx, JSValueConst this_val,
                             int argc, JSValueConst *argv)
{

    JSScope <10,10> scope(ctx);
    JSStreamData *s = (JSStreamData *) JS_GetOpaque2(ctx, this_val, js_stream_class_id);
    if (!s)
    {
       return JS_EXCEPTION;
    }
    for(int i=0; i<argc; i++)
    {
        if(!JS_IsString(argv[i]))
        {
            return JS_ThrowSyntaxError(ctx, "param must be string");
        } 
        auto str=scope.toStdStringView(argv[i]);
        s->stream->write("data",str.data(),str.size());

    }
    s->stream->write("end",NULL,0);

    return JS_UNDEFINED;
}
static JSValue js_stream_read(JSContext *ctx, JSValueConst this_val,
                              int argc, JSValueConst *argv)
{
    JSScope <10,10> scope(ctx);
    JSStreamData *s = (JSStreamData *) JS_GetOpaque2(ctx, this_val, js_stream_class_id);


    if (!s) return JS_EXCEPTION;

    std::string str=s->stream->read(1024*64);

    if(str.size())
    {
        return JS_NewStringLen(ctx,str.data(),str.size());
    }
    else return JS_NULL;
}

static JSClassDef js_stream_class = {
    "Stream",
    .finalizer = js_stream_finalizer,
};

static const JSCFunctionListEntry js_stream_proto_funcs[] = {
    // JS_CGETSET_MAGIC_DEF("x", js_stream_get_xy, js_stream_set_xy, 0),
    // JS_CGETSET_MAGIC_DEF("y", js_stream_get_xy, js_stream_set_xy, 1),
    JS_CFUNC_DEF("write", 0, js_stream_write),
    JS_CFUNC_DEF("end", 0, js_stream_end),
    JS_CFUNC_DEF("read", 0, js_stream_read),
    JS_CFUNC_DEF("on", 0, js_stream_on),
};

REF_getter<Stream> js_get_stream_CPP(JSContext *ctx, JSValueConst this_val)
{
    JSStreamData *s = (JSStreamData *)JS_GetOpaque2(ctx, this_val, js_stream_class_id);
    if (!s)
        return nullptr;
    return s->stream;
}
bool js_IsStream(JSContext* ctx, JSValueConst val)
{
    return JS_IsObject(val) && JS_GetClassID(val) == js_stream_class_id;
}

int js_init_stream_global(JSContext *ctx, JSValue & mtjs_obj)
{

    /* create the Stream class */
    JS_NewClassID(&js_stream_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_stream_class_id, &js_stream_class);

    JSValue stream_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, stream_proto, js_stream_proto_funcs, countof(js_stream_proto_funcs));

    JSValue EventEmitterStream_class = JS_NewCFunction2(ctx, js_EventEmitterStream_ctor, "EventEmitterStream", 1, JS_CFUNC_constructor, 0);

    JSValue ConstReadableStringStream_class = JS_NewCFunction2(ctx, js_ConstReadableStringStream_ctor, "ConstReadableStringStream", 1, JS_CFUNC_constructor, 0);
    JSValue WriteableStringStream_class = JS_NewCFunction2(ctx, js_WriteableStringStream_ctor, "WriteableStringStream", 0, JS_CFUNC_constructor, 0);
    JSValue FileReadableStream_class = JS_NewCFunction2(ctx, js_FileReadableStream_ctor, "FileReadableStream", 1, JS_CFUNC_constructor, 0);
    JSValue FileWriteableStream_class = JS_NewCFunction2(ctx, js_FileWriteableStream_ctor, "FileWriteableStream", 1, JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */
    JS_SetConstructor(ctx, EventEmitterStream_class, stream_proto);
    JS_SetConstructor(ctx, ConstReadableStringStream_class, stream_proto);
    JS_SetConstructor(ctx, WriteableStringStream_class, stream_proto);
    JS_SetConstructor(ctx, FileReadableStream_class, stream_proto);
    JS_SetConstructor(ctx, FileWriteableStream_class, stream_proto);
    JS_SetClassProto(ctx, js_stream_class_id, stream_proto);

    JSValue global_stream = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, global_stream, "EventEmitterStream", EventEmitterStream_class);
    JS_SetPropertyStr(ctx, global_stream, "ConstReadableStringStream", ConstReadableStringStream_class);
    JS_SetPropertyStr(ctx, global_stream, "WriteableStringStream",       WriteableStringStream_class);
    JS_SetPropertyStr(ctx, global_stream, "FileReadableStream",          FileReadableStream_class);
    JS_SetPropertyStr(ctx, global_stream, "FileWriteableStream",         FileWriteableStream_class);
    JS_SetPropertyStr(ctx, mtjs_obj, "stream", global_stream);

    // Освобождаем временные ссылки
    // JS_FreeValue(ctx, global_obj);

    return 0;

}
