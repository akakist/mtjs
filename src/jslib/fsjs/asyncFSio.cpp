#define _LARGEFILE64_SOURCE
#include "async_task.h"
#include "mtjs_opaque.h"

#include "quickjs.h"


#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <utime.h>
#include <stdint.h>
#include "malloc_debug.h"


JSClassID js_file_handle_class_id = 0;

typedef struct
    FileHandleData
{
    int fd;
    int closed;
} FileHandleData;

static void file_handle_finalizer(JSRuntime *rt, JSValue val)
{
    FileHandleData *fhd = (FileHandleData*)JS_GetOpaque(val, js_file_handle_class_id);
    if(fhd)
    {
        if(!fhd->closed && fhd->fd >= 0)
            close(fhd->fd);
            delete fhd;
    }
}


struct c_js_fs_access: public async_task
{
    int rv=-1;
    std::string errstr;
    std::string path;
    JSValue promise_data[2];

    c_js_fs_access(ListenerBase* l):async_task(l) {}
    void execute()
    {
        if(access(path.c_str(), F_OK) < 0)
        {
            errstr=strerror(errno);
            rv=-1;
            return;
        }
        rv=0;
    }
    void finalize(JSContext *ctx)
    {
        JSScope<10,10>  scope(ctx);
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
};
JSValue js_fs_access(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(!op) return JS_ThrowReferenceError(ctx, "!op");
    JSScope<10,10>  scope(ctx);
    if(argc!=1)
    {
        fprintf(stderr, "[%s] => not enough args => EXIT\n", __func__);
        return JS_ThrowTypeError(ctx,"fs.acsess requires (path)");
    }

    auto path=scope.toStdStringView(argv[0]);
    REF_getter<c_js_fs_access> f=new c_js_fs_access(op->listener);
    f->path=path;
    JSValue promise = JS_NewPromiseCapability(ctx, f->promise_data);
    DBG(memctl_add_object(promise, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

    if (JS_IsException(promise)) {
        return JS_ThrowInternalError(ctx,"JS_NewPromiseCapability error");
    }

    op->async_deque->push(f.get());
    return promise;
}

struct c_js_fs_appendFile: public async_task
{
    int rv=-1;
    std::string errstr;
    std::string path;
    std::string buf;
    JSValue promise_data[2];

    c_js_fs_appendFile(ListenerBase* l):async_task(l) {}
    void execute()
    {
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_APPEND, 0644);
        if(fd < 0)
        {
            rv=-1;
            errstr=strerror(errno);
            return;
        }
        else
        {
            ssize_t w = write(fd, buf.data(), buf.size());
            if(w<0)
            {
                rv=-1;
                errstr=strerror(errno);
                close(fd);
                return;
            }

        }
        close(fd);
        rv=0;
    }
    void finalize(JSContext *ctx)
    {
        JSScope<10,10>  scope(ctx);
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
};

JSValue js_fs_appendFile(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{

    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(!op) return JS_ThrowReferenceError(ctx, "!op");
    JSScope<10,10>  scope(ctx);
    if(argc!=2)
    {
        fprintf(stderr, "[%s] => not enough args => EXIT\n", __func__);
        return JS_ThrowTypeError(ctx,"fs.appendFile requires (path, string)");
    }

    auto path=scope.toStdStringView(argv[0]);
    auto buf=scope.toStdStringView(argv[1]);
    REF_getter<c_js_fs_appendFile> f=new c_js_fs_appendFile(op->listener);
    f->path=path;
    f->buf=buf;
    JSValue promise = JS_NewPromiseCapability(ctx, f->promise_data);
    DBG(memctl_add_object(promise, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

    if (JS_IsException(promise)) {
        return JS_ThrowInternalError(ctx,"JS_NewPromiseCapability error");
    }

    op->async_deque->push(f.get());
    return promise;
}
struct c_js_fs_chmod: public async_task
{
    int rv=-1;
    std::string errstr;
    std::string path;
    uint32_t mode;
    JSValue promise_data[2];

    c_js_fs_chmod(ListenerBase* l):async_task(l) {}
    void execute()
    {
        if(chmod(path.c_str(),mode))
        {
            rv=-1;
            errstr=strerror(errno);
            return;

        }
        rv=0;
    }
    void finalize(JSContext *ctx)
    {
        JSScope<10,10>  scope(ctx);
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
};

JSValue js_fs_chmod(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(!op) return JS_ThrowReferenceError(ctx, "!op");
    JSScope <10,10> scope(ctx);
    if(argc!=2)
    {
        fprintf(stderr, "[%s] => not enough args => EXIT\n", __func__);
        return JS_ThrowTypeError(ctx,"fs.chmod requires (path, mode)");
    }
    int32_t mode;
    if(JS_ToInt32(ctx,&mode,argv[1])<0)
    {
        return JS_ThrowSyntaxError(ctx,"invalid mode");
    }

    auto path=scope.toStdStringView(argv[0]);
    REF_getter<c_js_fs_chmod> f=new c_js_fs_chmod(op->listener);
    f->path=path;
    f->mode=mode;
    JSValue promise = JS_NewPromiseCapability(ctx, f->promise_data);
    DBG(memctl_add_object(promise, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

    if (JS_IsException(promise)) {
        return JS_ThrowInternalError(ctx,"JS_NewPromiseCapability error");
    }

    op->async_deque->push(f.get());
    return promise;
}

struct c_js_fs_chown: public async_task
{
    int rv=-1;
    std::string errstr;
    std::string path;
    uint32_t uid,gid;
    JSValue promise_data[2];

    c_js_fs_chown(ListenerBase* l):async_task(l) {}
    void execute()
    {
        if(chown(path.c_str(), uid, gid) < 0)
        {
            rv=-1;
            errstr=strerror(errno);
            return;
        }
        rv=0;
    }
    void finalize(JSContext *ctx)
    {
        JSScope <10,10> scope(ctx);
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
};

JSValue js_fs_chown(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(!op) return JS_ThrowReferenceError(ctx, "!op");
    JSScope <10,10> scope(ctx);
    if(argc!=3)
    {
        fprintf(stderr, "[%s] => not enough args => EXIT\n", __func__);
        return JS_ThrowTypeError(ctx,"fs.chown requires (path, uid, gid)");
    }
    int32_t uid;
    if(JS_ToInt32(ctx,&uid,argv[1])<0)
    {
        return JS_ThrowSyntaxError(ctx,"invalid mode");
    }
    int32_t gid;
    if(JS_ToInt32(ctx,&gid,argv[2])<0)
    {
        return JS_ThrowSyntaxError(ctx,"invalid mode");
    }

    auto path=scope.toStdStringView(argv[0]);
    REF_getter<c_js_fs_chown> f=new c_js_fs_chown(op->listener);
    f->path=path;
    f->uid=uid;
    f->gid=gid;
    JSValue promise = JS_NewPromiseCapability(ctx, f->promise_data);
    DBG(memctl_add_object(promise, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

    if (JS_IsException(promise)) {
        return JS_ThrowInternalError(ctx,"JS_NewPromiseCapability error");
    }

    op->async_deque->push(f.get());
    return promise;
}
struct c_js_fs_copyFile: public async_task
{
    int rv=-1;
    std::string errstr;
    std::string src,dst;
    JSValue promise_data[2];

    c_js_fs_copyFile(ListenerBase* l):async_task(l) {}
    void execute()
    {
        int in_fd=open(src.c_str(),O_RDONLY);
        if(in_fd<0)
        {
            rv=-1;
            errstr=strerror(errno);
        }
        int out_fd=open(dst.c_str(),O_WRONLY|O_CREAT|O_TRUNC,0644);
        if(out_fd<0)
        {
            rv=-1;
            errstr=strerror(errno);
            close(in_fd);
            return;
        }
        char buf[8192];
        while(1)
        {
            ssize_t r=read(in_fd, buf, sizeof(buf));
            if(r<0)
            {
                rv=-1;
                errstr=strerror(errno);
                close(in_fd);
                close(out_fd);
                return;
            }
            if(r==0) break;
            ssize_t wpos=0;
            while(wpos<r)
            {
                ssize_t w=write(out_fd, buf+wpos, r-wpos);
                if(w<0)
                {
                    rv=-1;
                    errstr=strerror(errno);
                    close(in_fd);
                    close(out_fd);
                    return;
                }
                wpos+=w;
            }
        }
        close(in_fd);
        close(out_fd);
        rv=0;
    }
    void finalize(JSContext *ctx)
    {
        JSScope <10,10> scope(ctx);
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
};

JSValue js_fs_copyFile(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(!op) return JS_ThrowReferenceError(ctx, "!op");
    JSScope <10,10> scope(ctx);
    if(argc!=2)
    {
        fprintf(stderr, "[%s] => not enough args => EXIT\n", __func__);
        return JS_ThrowTypeError(ctx,"fs.truncate requires (path mode)");
    }

    auto src=scope.toStdStringView(argv[0]);
    auto dst=scope.toStdStringView(argv[1]);
    REF_getter<c_js_fs_copyFile> f=new c_js_fs_copyFile(op->listener);
    f->src=src;
    f->dst=dst;
    JSValue promise = JS_NewPromiseCapability(ctx, f->promise_data);
    DBG(memctl_add_object(promise, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

    if (JS_IsException(promise)) {
        return JS_ThrowInternalError(ctx,"JS_NewPromiseCapability error");
    }

    op->async_deque->push(f.get());
    return promise;
}

struct c_js_fs_mkdir: public async_task
{
    int rv=-1;
    std::string errstr;
    std::string path;
    std::optional<int32_t> mode;
    JSValue promise_data[2];

    c_js_fs_mkdir(ListenerBase* l):async_task(l) {}
    void execute()
    {
        if(mkdir(path.c_str(),*mode)<0)
        {
            rv=-1;
        }
        else
            rv=0;
    }
    void finalize(JSContext *ctx)
    {
        JSScope <10,10> scope(ctx);
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
};

JSValue js_fs_mkdir(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(!op) return JS_ThrowReferenceError(ctx, "!op");
    JSScope <10,10> scope(ctx);
    if(argc!=2)
    {
        fprintf(stderr, "[%s] => not enough args => EXIT\n", __func__);
        return JS_ThrowTypeError(ctx,"fs.truncate requires (path mode)");
    }
    std::optional<int32_t>m;
    int32_t mode;
    if(JS_ToInt32(ctx,&mode,argv[1])<0)
    {
        return JS_ThrowSyntaxError(ctx,"invalid mode");
    }
    m.emplace(mode);

    auto path=scope.toStdStringView(argv[0]);
    REF_getter<c_js_fs_mkdir> f=new c_js_fs_mkdir(op->listener);
    f->path=path;
    f->mode=m;
    JSValue promise = JS_NewPromiseCapability(ctx, f->promise_data);
    DBG(memctl_add_object(promise, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

    if (JS_IsException(promise)) {
        return JS_ThrowInternalError(ctx,"JS_NewPromiseCapability error");
    }

    op->async_deque->push(f.get());
    return promise;

}
struct c_js_fs_mkdtemp: public async_task
{
    int rv=-1;
    std::string errstr;
    std::string path;
    std::string ret;
    JSValue promise_data[2];

    c_js_fs_mkdtemp(ListenerBase* l):async_task(l) {}
    void execute()
    {
        char *s =strdup(path.c_str());
        char *res = mkdtemp(s);
        if(!res)
        {
            free(s);
            rv=-1;
            errstr=strerror(errno);
            return;
        }
        ret=res;
        free(s);
        rv=0;
    }
    void finalize(JSContext *ctx)
    {
        JSScope <10,10> scope(ctx);
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
            JSValue z=JS_NewStringLen(ctx, ret.data(),ret.size());
            DBG(memctl_add_object(z, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

            auto res=JS_Call(ctx, promise_data[0], JS_UNDEFINED, 1, &z);
            DBG(memctl_add_object(res, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

            scope.addValue(res);
        }
        qjs::free_promise_callbacks(ctx,promise_data);

    }
};

JSValue js_fs_mkdtemp(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{

    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(!op) return JS_ThrowReferenceError(ctx, "!op");
    JSScope<10,10> scope(ctx);
    if(argc!=1)
    {
        fprintf(stderr, "[%s] => not enough args => EXIT\n", __func__);
        return JS_ThrowTypeError(ctx,"fs.truncate requires (path, len)");
    }
    auto path=scope.toStdStringView(argv[0]);
    REF_getter<c_js_fs_mkdtemp> f=new c_js_fs_mkdtemp(op->listener);
    f->path=path;
    f->path+="XXXXXX";
    JSValue promise = JS_NewPromiseCapability(ctx, f->promise_data);
    DBG(memctl_add_object(promise, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

    if (JS_IsException(promise)) {
        return JS_ThrowInternalError(ctx,"JS_NewPromiseCapability error");
    }

    op->async_deque->push(f.get());
    return promise;
}
struct filehandle_readFile: public async_task
{
    int rv=-1;
    std::string errstr;
    std::string path;
    std::string ret;
    JSValue promise_data[2];

    filehandle_readFile(ListenerBase* l):async_task(l) {}
    void execute()
    {
        int fd=open(path.c_str(),O_RDONLY);
        if(fd<0) {
            rv=-1;
            errstr=strerror(errno);
            return;
        }
        else
        {
            struct stat st;
            if(fstat(fd,&st)==0 && st.st_size>0)
            {
                ret.resize(st.st_size);

                {
                    ssize_t r=read(fd, ret.data(), st.st_size);
                    if(r<0)
                    {
                        rv=-1;
                        errstr=strerror(errno);
                    }
                    else rv=0;
                }
            }
            close(fd);
        }


    }
    void finalize(JSContext *ctx)
    {
        JSScope<10,10> scope(ctx);
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
            JSValue z=JS_NewStringLen(ctx, ret.data(),ret.size());
            DBG(memctl_add_object(z, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

            auto res=JS_Call(ctx, promise_data[0], JS_UNDEFINED, 1, &z);
            DBG(memctl_add_object(res, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

            scope.addValue(res);
        }
        qjs::free_promise_callbacks(ctx,promise_data);

    }
};

JSValue js_fs_readFile(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(!op) return JS_ThrowReferenceError(ctx, "!op");
    JSScope<10,10> scope(ctx);
    if(argc!=1)
    {
        fprintf(stderr, "[%s] => not enough args => EXIT\n", __func__);
        return JS_ThrowTypeError(ctx,"fs.truncate requires (path, len)");
    }
    auto path=scope.toStdStringView(argv[0]);
    REF_getter<filehandle_readFile> f=new filehandle_readFile(op->listener);
    f->path=path;
    JSValue promise = JS_NewPromiseCapability(ctx, f->promise_data);
    DBG(memctl_add_object(promise, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

    if (JS_IsException(promise)) {
        return JS_ThrowInternalError(ctx,"JS_NewPromiseCapability error");
    }

    op->async_deque->push(f.get());
    return promise;

}

struct filehandle_readdir: public async_task
{
    int rv=-1;
    std::string errstr;
    std::string path;
    JSValue promise_data[2];
    std::vector<std::string> res;

    filehandle_readdir(ListenerBase* l):async_task(l) {}
    void execute()
    {
        DIR*d = opendir(path.c_str());
        if(!d)
        {
            rv=-1;
            errstr=strerror(errno);
            return;
        }
        rv=0;
        {
            struct dirent* de;
            while((de = readdir(d)))
            {
                res.push_back(de->d_name);
            }
            closedir(d);
        }
    }
    void finalize(JSContext *ctx)
    {
        JSScope<10,10>  scope(ctx);
        if(rv<0)
        {
            JSValue r=JS_NewError(ctx);
            DBG(memctl_add_object(r, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

            scope.addValue(r);
            JS_DefinePropertyValueStr(ctx, r, "message",
                                      JS_NewString(ctx, errstr.c_str()),
                                      JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
            auto ret=JS_Call(ctx, promise_data[1], JS_UNDEFINED, 1, &r);
            DBG(memctl_add_object(ret, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

            scope.addValue(ret);

        }
        else
        {
            JSValue arr = JS_NewArray(ctx);
            DBG(memctl_add_object(arr, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

            scope.addValue(arr);
            for(int i = 0; i < res.size(); i++)
            {
                JSValue s = JS_NewString(ctx, res[i].c_str());
                DBG(memctl_add_object(s, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

                JS_SetPropertyUint32(ctx, arr, (uint32_t)i, s);
            }
            auto res=JS_Call(ctx, promise_data[0], JS_UNDEFINED, 1, &arr);
            DBG(memctl_add_object(res, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

            scope.addValue(res);
        }
        qjs::free_promise_callbacks(ctx,promise_data);

    }
};


JSValue js_fs_readdir(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(!op) return JS_ThrowReferenceError(ctx, "!op");
    JSScope<10,10>  scope(ctx);
    if(argc!=1)
    {
        fprintf(stderr, "[%s] => not enough args => EXIT\n", __func__);
        return JS_ThrowTypeError(ctx,"fs.truncate requires (path, len)");
    }
    auto path=scope.toStdStringView(argv[0]);
    REF_getter<filehandle_readdir> f=new filehandle_readdir(op->listener);
    f->path=path;
    JSValue promise = JS_NewPromiseCapability(ctx, f->promise_data);
    DBG(memctl_add_object(promise, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

    if (JS_IsException(promise)) {
        return JS_ThrowInternalError(ctx,"JS_NewPromiseCapability error");
    }

    op->async_deque->push(f.get());
    return promise;
}

struct filehandle_rmdir: public async_task
{
    int rv=-1;
    std::string errstr;
    std::string path;
    JSValue promise_data[2];

    filehandle_rmdir(ListenerBase* l):async_task(l) {}
    void execute()
    {
        if(::rmdir(path.c_str())<0)
        {
            rv=-1;
            errstr=strerror(errno);
        }
        else rv=0;

    }
    void finalize(JSContext *ctx)
    {
        JSScope<10,10>  scope(ctx);
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
};

JSValue js_fs_rmdir(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(!op) return JS_ThrowReferenceError(ctx, "!op");
    JSScope<10,10>  scope(ctx);
    if(argc!=1)
    {
        fprintf(stderr, "[%s] => not enough args => EXIT\n", __func__);
        return JS_ThrowTypeError(ctx,"fs.truncate requires (path, len)");
    }
    auto path=scope.toStdStringView(argv[0]);
    REF_getter<filehandle_rmdir> f=new filehandle_rmdir(op->listener);
    f->path=path;
    JSValue promise = JS_NewPromiseCapability(ctx, f->promise_data);
    DBG(memctl_add_object(promise, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

    if (JS_IsException(promise)) {
        return JS_ThrowInternalError(ctx,"JS_NewPromiseCapability error");
    }

    op->async_deque->push(f.get());
    return promise;
}

struct filehandle_rename: public async_task
{
    int rv=-1;
    std::string errstr;
    std::string path1,path2;
    JSValue promise_data[2];

    filehandle_rename(ListenerBase* l):async_task(l) {}
    void execute()
    {
        if(::rename(path1.c_str(),path2.c_str())<0)
        {
            rv=-1;
            errstr=strerror(errno);
        }
        else rv=0;

    }
    void finalize(JSContext *ctx)
    {
        JSScope<10,10>  scope(ctx);
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
            DBG(memctl_add_object(res, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

            scope.addValue(res);
        }
        qjs::free_promise_callbacks(ctx,promise_data);

    }
};

JSValue js_fs_rename(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(!op) return JS_ThrowReferenceError(ctx, "!op");
    JSScope<10,10>  scope(ctx);
    if(argc!=2)
    {
        fprintf(stderr, "[%s] => not enough args => EXIT\n", __func__);
        return JS_ThrowTypeError(ctx,"fs.rename requires (path1, path2)");
    }
    auto path1=scope.toStdStringView(argv[0]);
    auto path2=scope.toStdStringView(argv[2]);
    REF_getter<filehandle_rename> f=new filehandle_rename(op->listener);
    f->path1=path1;
    f->path2=path2;
    JSValue promise = JS_NewPromiseCapability(ctx, f->promise_data);
    DBG(memctl_add_object(promise, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

    if (JS_IsException(promise)) {
        return JS_ThrowInternalError(ctx,"JS_NewPromiseCapability error");
    }

    op->async_deque->push(f.get());
    return promise;
}
struct filehandle_unlink: public async_task
{
    int rv=-1;
    std::string errstr;
    std::string path;
    JSValue promise_data[2];

    filehandle_unlink(ListenerBase* l):async_task(l) {}
    void execute()
    {
        if(::unlink(path.c_str())<0)
        {
            rv=-1;
            errstr=strerror(errno);
        }
        else rv=0;

    }
    void finalize(JSContext *ctx)
    {
        JSScope<10,10>  scope(ctx);
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
};

JSValue js_fs_unlink(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{

    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(!op) return JS_ThrowReferenceError(ctx, "!op");
    JSScope<10,10>  scope(ctx);
    if(argc!=1)
    {
        fprintf(stderr, "[%s] => not enough args => EXIT\n", __func__);
        return JS_ThrowTypeError(ctx,"fs.truncate requires (path, len)");
    }
    auto path=scope.toStdStringView(argv[0]);
    REF_getter<filehandle_unlink> f=new filehandle_unlink(op->listener);
    f->path=path;
    JSValue promise = JS_NewPromiseCapability(ctx, f->promise_data);
    DBG(memctl_add_object(promise, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

    if (JS_IsException(promise)) {
        return JS_ThrowInternalError(ctx,"JS_NewPromiseCapability error");
    }

    op->async_deque->push(f.get());
    return promise;

}

struct filehandle_stat: public async_task
{
    int rv=-1;
    std::string errstr;
    std::string path;
    struct stat st;
    JSValue promise_data[2];

    filehandle_stat(ListenerBase* l):async_task(l) {}
    void execute()
    {
        if(::stat(path.c_str(), &st)<0)
        {
            rv=-1;
            errstr=strerror(errno);
        }
        else rv=0;

    }
    void finalize(JSContext *ctx)
    {
        JSScope<10,10>  scope(ctx);
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
            JSValue obj = JS_NewObject(ctx);
            DBG(memctl_add_object(obj, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

            scope.addValue(obj);
            JS_DefinePropertyValueStr(ctx, obj, "dev", JS_NewInt64(ctx, st.st_dev), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "ino", JS_NewInt64(ctx, st.st_ino), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "mode", JS_NewInt32(ctx, (int32_t)st.st_mode), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "nlink", JS_NewInt64(ctx, st.st_nlink), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "uid", JS_NewInt32(ctx, st.st_uid), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "gid", JS_NewInt32(ctx, st.st_gid), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "rdev", JS_NewInt64(ctx, st.st_rdev), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "size", JS_NewInt64(ctx, st.st_size), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "blksize", JS_NewInt64(ctx, st.st_blksize), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "blocks", JS_NewInt64(ctx, st.st_blocks), JS_PROP_C_W_E);

            JS_DefinePropertyValueStr(ctx, obj, "atime", JS_NewInt64(ctx, st.st_atime), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "mtime", JS_NewInt64(ctx, st.st_mtime), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "ctime", JS_NewInt64(ctx, st.st_ctime), JS_PROP_C_W_E);


            auto res=JS_Call(ctx, promise_data[0], JS_UNDEFINED, 1, &obj);
            scope.addValue(res);
        }
        qjs::free_promise_callbacks(ctx,promise_data);

    }
};


JSValue js_fs_stat(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(!op) return JS_ThrowReferenceError(ctx, "!op");
    JSScope<10,10>  scope(ctx);
    if(argc!=1)
    {
        fprintf(stderr, "[%s] => not enough args => EXIT\n", __func__);
        return JS_ThrowTypeError(ctx,"fs.truncate requires (path, len)");
    }
    auto path=scope.toStdStringView(argv[0]);
    REF_getter<filehandle_stat> f=new filehandle_stat(op->listener);
    f->path=path;
    JSValue promise = JS_NewPromiseCapability(ctx, f->promise_data);
    DBG(memctl_add_object(promise, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

    if (JS_IsException(promise)) {
        return JS_ThrowInternalError(ctx,"JS_NewPromiseCapability error");
    }

    op->async_deque->push(f.get());
    return promise;

}
struct filehandle_symlink: public async_task
{
    int rv=-1;
    std::string errstr;
    std::string path1,path2;
    JSValue promise_data[2];

    filehandle_symlink(ListenerBase* l):async_task(l) {}
    void execute()
    {
        if(symlink(path1.c_str(), path2.c_str())<0)
        {
            rv=-1;
            errstr=strerror(errno);
        }
        else rv=0;

    }
    void finalize(JSContext *ctx)
    {
        JSScope<10,10>  scope(ctx);
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
            DBG(memctl_add_object(res, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

            scope.addValue(res);
        }
        qjs::free_promise_callbacks(ctx,promise_data);

    }
};

JSValue js_fs_symlink(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(!op) return JS_ThrowReferenceError(ctx, "!op");
    JSScope<10,10>  scope(ctx);
    if(argc!=2)
    {
        fprintf(stderr, "[%s] => not enough args => EXIT\n", __func__);
        return JS_ThrowTypeError(ctx,"fs.truncate requires (path, len)");
    }
    auto path1=scope.toStdStringView(argv[0]);
    auto path2=scope.toStdStringView(argv[1]);
    REF_getter<filehandle_symlink> f=new filehandle_symlink(op->listener);
    f->path1=path1;
    f->path1=path1;
    JSValue promise = JS_NewPromiseCapability(ctx, f->promise_data);
    DBG(memctl_add_object(promise, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

    if (JS_IsException(promise)) {
        return JS_ThrowInternalError(ctx,"JS_NewPromiseCapability error");
    }

    op->async_deque->push(f.get());
    return promise;
}
struct filehandle_truncate: public async_task
{
    int rv=-1;
    std::string errstr;
    std::string path;
    int64_t size;
    JSValue promise_data[2];

    filehandle_truncate(ListenerBase* l):async_task(l) {}
    void execute()
    {
        if(truncate(path.c_str(), size)<0)
        {
            rv=-1;
            errstr=strerror(errno);
        }
        else rv=0;
    }
    void finalize(JSContext *ctx)
    {
        JSScope<10,10>  scope(ctx);
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
};

JSValue js_fs_truncate(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(!op) return JS_ThrowReferenceError(ctx, "!op");
    JSScope<10,10>  scope(ctx);
    if(argc!=2)
    {
        fprintf(stderr, "[%s] => not enough args => EXIT\n", __func__);
        return JS_ThrowTypeError(ctx,"fs.truncate requires (path, len)");
    }
    auto path=scope.toStdStringView(argv[0]);
    int64_t length=0;
    if(JS_ToInt64(ctx, &length, argv[1]))
    {
        fprintf(stderr, "[%s] => parse length => EXIT\n", __func__);
        return JS_EXCEPTION;
    }
    REF_getter<filehandle_truncate> f=new filehandle_truncate(op->listener);
    f->path=path;
    f->size=length;
    JSValue promise = JS_NewPromiseCapability(ctx, f->promise_data);
    DBG(memctl_add_object(promise, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

    if (JS_IsException(promise)) {
        return JS_ThrowInternalError(ctx,"JS_NewPromiseCapability error");
    }

    op->async_deque->push(f.get());
    return promise;
}
struct filehandle_utimes: public async_task
{
    int rv=-1;
    std::string errstr;
    std::string path;
    int64_t a,m;
    JSValue promise_data[2];

    filehandle_utimes(ListenerBase* l):async_task(l) {}
    void execute()
    {

        struct utimbuf ut;
        ut.actime=a;
        ut.modtime=m;
        if(utime(path.c_str(),&ut)<0)
        {
            rv=-1;
            errstr=strerror(errno);
        }
        else rv=0;


    }
    void finalize(JSContext *ctx)
    {
        JSScope<10,10>  scope(ctx);
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
};

JSValue js_fs_utimes(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(!op) return JS_ThrowReferenceError(ctx, "!op");
    JSScope<10,10>  scope(ctx);
    if(argc!=3)
    {
        fprintf(stderr, "[%s] => not enough args => EXIT\n", __func__);
        return JS_ThrowTypeError(ctx,"fs.utimes requires (path, atime, mtime)");
    }
    auto  path=scope.toStdStringView(argv[0]);

    int64_t a=0,m=0;
    if(JS_ToInt64(ctx,&a,argv[1])||JS_ToInt64(ctx,&m,argv[2]))
    {
        fprintf(stderr, "[%s] => parse atime/mtime => EXIT\n", __func__);
        return JS_EXCEPTION;
    }

    REF_getter<filehandle_utimes> f=new filehandle_utimes(op->listener);
    f->path=path;
    f->a=a;
    f->m=m;
    JSValue promise = JS_NewPromiseCapability(ctx, f->promise_data);
    DBG(memctl_add_object(promise, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

    if (JS_IsException(promise)) {
        return JS_ThrowInternalError(ctx,"JS_NewPromiseCapability error");
    }

    op->async_deque->push(f.get());
    return promise;
}
struct filehandle_writeFile: public async_task
{
    int rv=-1;
    std::string errstr;
    std::string path;
    std::string body;
    JSValue promise_data[2];

    filehandle_writeFile(ListenerBase* l):async_task(l) {}
    void execute()
    {

        int fd=open(path.c_str(),O_CREAT|O_WRONLY|O_TRUNC,0644);
        rv=fd;
        if(fd<0)
        {
            rv=errno;
            errstr=strerror(errno);
            return;
        }
        ssize_t w=write(fd, body.data(), body.size());
        if(w<0) rv=errno;
        close(fd);

    }
    void finalize(JSContext *ctx)
    {
        JSScope<10,10>  scope(ctx);
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
};

JSValue js_fs_writeFile(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(!op) return JS_ThrowReferenceError(ctx, "!op");
    JSScope<10,10>  scope(ctx);
    if(argc<2)
    {
        fprintf(stderr, "[%s] => not enough args => EXIT\n", __func__);
        return JS_ThrowTypeError(ctx,"fs.writeFile requires (path, data)");
    }
    auto path=scope.toStdStringView(argv[0]);

    auto body=scope.toStdStringView(argv[1]);

    REF_getter<filehandle_writeFile> f=new filehandle_writeFile(op->listener);
    f->body=body;
    f->path=path;
    JSValue promise = JS_NewPromiseCapability(ctx, f->promise_data);
    DBG(memctl_add_object(promise, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

    if (JS_IsException(promise)) {
        return JS_ThrowInternalError(ctx,"JS_NewPromiseCapability error");
    }

    op->async_deque->push(f.get());
    return promise;
}

struct filehandle_open: public async_task
{
    int fd;
    int rv=-1;
    std::string errstr;
    std::string path;
    uint32_t fl;
    uint32_t mode;
    JSValue fhObj;
    JSValue promise_data[2];

    filehandle_open(ListenerBase* l):async_task(l) {}
    void execute()
    {
        fd=::open(path.c_str(),fl,mode);
        if(fd<0)
            errstr=strerror(errno);
    }
    void finalize(JSContext *ctx)
    {
        JSScope<10,10>  scope(ctx);
        if(fd<0)
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
            FileHandleData *fhd =new FileHandleData;
            fhd->fd = fd;
            fhd->closed = 0;
            JS_SetOpaque(fhObj, fhd);

            auto res=JS_Call(ctx, promise_data[0], JS_UNDEFINED, 1, &fhObj);
            scope.addValue(res);


        }
        JS_FreeValue(ctx,fhObj);
        qjs::free_promise_callbacks(ctx,promise_data);



    }
};


JSValue js_fs_open(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(!op) return JS_ThrowReferenceError(ctx, "!op");
    JSScope<10,10> scope(ctx);
    if(argc<2)
    {
        fprintf(stderr, "[%s] => not enough args => EXIT\n", __func__);
        return JS_ThrowTypeError(ctx,"fs.open requires (path, flags[, mode])");
    }
    auto path=scope.toStdStringView(argv[0]);
    uint32_t fl=0;
    if(JS_ToUint32(ctx,&fl,argv[1]))
    {
        fprintf(stderr, "[%s] => parse flags => EXIT\n", __func__);
        return JS_EXCEPTION;
    }
    uint32_t mode=0;
    if(argc==3)
    {
        if(JS_ToUint32(ctx,&mode,argv[2]))
        {
            fprintf(stderr, "[%s] => parse mode => EXIT\n", __func__);
            return JS_EXCEPTION;
        }

    }
    REF_getter<filehandle_open> f=new filehandle_open(op->listener);
    f->path=path;
    f->fl=fl;
    f->mode=mode;
    JSValue fh = JS_NewObjectClass(ctx,js_file_handle_class_id);
    f->fhObj=fh;
    JSValue promise = JS_NewPromiseCapability(ctx, f->promise_data);
    DBG(memctl_add_object(promise, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

    if (JS_IsException(promise)) {
        return JS_ThrowInternalError(ctx,"JS_NewPromiseCapability error");
    }

    op->async_deque->push(f.get());
    return promise;

}



struct filehandle_read: public async_task
{
    int fd;
    int rv=-1;
    std::string errstr;
    std::string buf;
    int64_t pz;
    int64_t size;
    JSValue promise_data[2];

    filehandle_read(ListenerBase* l):async_task(l) {}
    void execute()
    {
        std::string b;
        b.resize(size);
        if(pz!=-1)
            lseek64(fd,pz,SEEK_SET);
        rv=::read(fd,b.data(),b.size());
        if(rv<0)
            errstr=strerror(errno);
        buf=b.substr(0,rv);
    }
    void finalize(JSContext *ctx)
    {
        JSScope<10,10>  scope(ctx);
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
            JSValue r = JS_NewStringLen(ctx,buf.data(),buf.size());
            DBG(memctl_add_object(r, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

            scope.addValue(r);
            auto ret=JS_Call(ctx, promise_data[0], JS_UNDEFINED, 1, &r);
            scope.addValue(ret);

        }
        qjs::free_promise_callbacks(ctx,promise_data);


    }
};



JSValue js_filehandle_read(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(!op) return JS_ThrowReferenceError(ctx, "!op");

    FileHandleData* fhd = (FileHandleData*)JS_GetOpaque(this_val, js_file_handle_class_id);
    if(!fhd || fhd->closed)
        return JS_ThrowTypeError(ctx, "FileHandle invalid or closed");

    if(argc<1)
        return JS_ThrowSyntaxError(ctx,"size must be specified");
    int64_t size;
    if(JS_ToInt64(ctx, &size, argv[0])<0)
        return JS_ThrowSyntaxError(ctx,"size must be int64");
    int64_t pz=-1;
    if(argc==2)
    {
        if(JS_ToInt64(ctx, &pz, argv[1])<0)
            return JS_ThrowSyntaxError(ctx,"fileoffset must be int64");

    }

    REF_getter<filehandle_read> fr=new filehandle_read(op->listener);
    fr->fd=fhd->fd;
    fr->size=size;
    fr->pz=pz;

    JSValue promise = JS_NewPromiseCapability(ctx, fr->promise_data);
    DBG(memctl_add_object(promise, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

    if (JS_IsException(promise)) {
        return JS_ThrowInternalError(ctx,"JS_NewPromiseCapability error");
    }

    op->async_deque->push(fr.get());
    return promise;
}

struct filehandle_write: public async_task
{
    int fd;
    int rv=-1;
    std::string errstr;
    std::string buf;
    int64_t pz;
    JSValue promise_data[2];

    filehandle_write(ListenerBase* l):async_task(l) {}
    void execute()
    {
        if(pz!=-1)
            lseek64(fd,pz,SEEK_SET);
        rv=::write(fd,buf.data(),buf.size());
        if(rv<0)
            errstr=strerror(errno);
    }
    void finalize(JSContext *ctx)
    {
        JSScope<10,10>  scope(ctx);
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
            JSValue r = JS_NewUint32(ctx,rv);
            auto ret=JS_Call(ctx, promise_data[0], JS_UNDEFINED, 1, &r);
            scope.addValue(ret);

        }
        qjs::free_promise_callbacks(ctx,promise_data);



    }
};


JSValue js_filehandle_write(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(!op) return JS_ThrowReferenceError(ctx, "!op");


    JSScope<10,10>  scope(ctx);
    FileHandleData* fhd = (FileHandleData*)JS_GetOpaque(this_val, js_file_handle_class_id);
    if(!fhd || fhd->closed)
    {
        fprintf(stderr, "[js_filehandle_write] => filehandle invalid or closed => EXIT\n");
        return JS_ThrowTypeError(ctx, "FileHandle closed or invalid");
    }
    std::string buf;
    if(argc>0)
    {
        if(JS_IsString(argv[0]))
            buf=scope.toStdStringView(argv[0]);
        else return JS_ThrowSyntaxError(ctx, "argv[0] must be String");
    }
    int64_t pz=-1;
    if(argc>1)
    {
        if(JS_ToInt64(ctx,&pz,argv[1])<0)
            return JS_ThrowSyntaxError(ctx, "second param is optional must be file offset from beginning (SEEK_SET)");
    }

    REF_getter<filehandle_write> fw=new filehandle_write(op->listener);
    fw->buf=std::move(buf);
    fw->pz=pz;
    fw->fd=fhd->fd;

    JSValue promise = JS_NewPromiseCapability(ctx, fw->promise_data);
    DBG(memctl_add_object(promise, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

    if (JS_IsException(promise)) {
        return JS_ThrowInternalError(ctx,"JS_NewPromiseCapability error");
    }

    op->async_deque->push(fw.get());
    return promise;


}



struct filehandle_close: public async_task
{
    int fd;
    int rv=-1;
    std::string errstr;
    JSValue promise_data[2];

    filehandle_close(ListenerBase* l):async_task(l) {}
    void execute()
    {
        rv=::close(fd);
        if(rv<0)
            errstr=strerror(errno);
    }
    void finalize(JSContext *ctx)
    {
        JSScope<10,10> scope(ctx);
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
            JSValue r = JS_NewUint32(ctx,rv);
            auto ret=JS_Call(ctx, promise_data[0], JS_UNDEFINED, 1, &r);
            scope.addValue(ret);

        }
        qjs::free_promise_callbacks(ctx,promise_data);



    }
};
JSValue js_filehandle_close(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    mtjs_opaque *op=(mtjs_opaque *)JS_GetContextOpaque(ctx);
    if(!op) return JS_ThrowReferenceError(ctx, "!op");

    FileHandleData*fhd=(FileHandleData*)JS_GetOpaque(this_val, js_file_handle_class_id);
    if(!fhd||fhd->closed)
    {
        return JS_ThrowInternalError(ctx, "if(!fhd||fhd->closed)");
    }

    REF_getter<filehandle_close> fc=new filehandle_close(op->listener);
    fc->fd=fhd->fd;
    JSValue promise = JS_NewPromiseCapability(ctx, fc->promise_data);
    DBG(memctl_add_object(promise, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

    if (JS_IsException(promise)) {
        return JS_ThrowInternalError(ctx,"JS_NewPromiseCapability error");
    }

    op->async_deque->push(fc.get());
    return promise;
}
#define OS_FLAG(x) JS_PROP_INT32_DEF(#x, x, JS_PROP_CONFIGURABLE )


static const JSCFunctionListEntry filehandle_proto_funcs[] =
{
    JS_CFUNC_DEF("read",   4, js_filehandle_read),
    JS_CFUNC_DEF("write",  4, js_filehandle_write),
    JS_CFUNC_DEF("close",  0, js_filehandle_close),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "FileHandle", JS_PROP_CONFIGURABLE),
};


static const JSCFunctionListEntry fs_funcs[] =
{
    JS_CFUNC_DEF("access",      1, js_fs_access),
    JS_CFUNC_DEF("appendFile",  2, js_fs_appendFile),
    JS_CFUNC_DEF("chmod",       2, js_fs_chmod),
    JS_CFUNC_DEF("chown",       3, js_fs_chown),
    JS_CFUNC_DEF("copyFile",    2, js_fs_copyFile),
    JS_CFUNC_DEF("mkdir",       2, js_fs_mkdir),
    JS_CFUNC_DEF("mkdtemp",     1, js_fs_mkdtemp),
    JS_CFUNC_DEF("readFile",    1, js_fs_readFile),
    JS_CFUNC_DEF("readdir",     1, js_fs_readdir),
    JS_CFUNC_DEF("rmdir",       1, js_fs_rmdir),
    JS_CFUNC_DEF("rename",      2, js_fs_rename),
    JS_CFUNC_DEF("unlink",      1, js_fs_unlink),
    JS_CFUNC_DEF("stat",        1, js_fs_stat),
    JS_CFUNC_DEF("symlink",     2, js_fs_symlink),
    JS_CFUNC_DEF("truncate",    2, js_fs_truncate),
    JS_CFUNC_DEF("utimes",      3, js_fs_utimes),
    JS_CFUNC_DEF("writeFile",   2, js_fs_writeFile),
    JS_CFUNC_DEF("open",        3, js_fs_open),
    OS_FLAG(O_RDONLY),
    OS_FLAG(O_WRONLY),
    OS_FLAG(O_RDWR),
    OS_FLAG(O_APPEND),
    OS_FLAG(O_CREAT),
    OS_FLAG(O_EXCL),
    OS_FLAG(O_TRUNC),
#if defined(_WIN32)
    OS_FLAG(O_BINARY),
    OS_FLAG(O_TEXT),
#endif
    OS_FLAG(S_IFMT),
    OS_FLAG(S_IFIFO),
    OS_FLAG(S_IFCHR),
    OS_FLAG(S_IFDIR),
    OS_FLAG(S_IFBLK),
    OS_FLAG(S_IFREG),
#if !defined(_WIN32)
    OS_FLAG(S_IFSOCK),
    OS_FLAG(S_IFLNK),
    OS_FLAG(S_ISGID),
    OS_FLAG(S_ISUID),
#endif


    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "IO", JS_PROP_CONFIGURABLE),
};



void register_fh_class(JSContext* ctx)
{
    XTRY;
    MUTEX_INSPECTOR;
    {
        JS_NewClassID(&js_file_handle_class_id);
        JSClassDef def;
        memset(&def,0,sizeof(def));
        def.class_name="FileHandle";
        def.finalizer=file_handle_finalizer;

        JS_NewClass(JS_GetRuntime(ctx), js_file_handle_class_id, &def);

        JSValue proto=JS_NewObject(ctx);
        JS_SetPropertyFunctionList(ctx, proto, filehandle_proto_funcs, sizeof(filehandle_proto_funcs)/sizeof(JSCFunctionListEntry));
        JS_SetClassProto(ctx, js_file_handle_class_id, proto);

    }

    XPASS;

}

void js_init_fs(JSContext *ctx, JSValue mtjs_obj)
{
    register_fh_class(ctx);
    JSValue m = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, m, fs_funcs, sizeof(fs_funcs)/sizeof(JSCFunctionListEntry));
    JS_SetPropertyStr(ctx, mtjs_obj, "fs", m);

}
