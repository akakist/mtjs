#include <quickjs.h>
#include <string>
#include "httpConnection.h"
#include "http_server_response.h"
#include "common/jsscope.h"
#include "common/js_tools.h"
#include "malloc_debug.h"

static JSClassID js_response_class_id;

class JS_HTTP_Response {
public:
    REF_getter<HTTP_ResponseP> respP;
    // REF_getter<EventEmitter> emitter;
    bool status_set_allowed=true;



    JS_HTTP_Response(const REF_getter<HTTP_ResponseP>& r) : respP(r) {
        DBG(iUtils->mem_add_ptr("JS_HTTP_Response",this));

        // DBG(logErr2("JS_HTTP_Response()"));
    }
    ~JS_HTTP_Response()
    {
        DBG(iUtils->mem_remove_ptr("JS_HTTP_Response",this));
        // DBG(logErr2("~JS_HTTP_Response()"));

    }
};



static void js_response_finalizer(JSRuntime* rt, JSValue val) {
    JS_HTTP_Response* req = static_cast<JS_HTTP_Response*>(JS_GetOpaque(val, js_response_class_id));
    if (req) {
        delete req;
    }
}

static JSClassDef js_response_class = {
    "Response",
    .finalizer = js_response_finalizer,
};


static JSValue js_set_header(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    MUTEX_INSPECTOR;
    JS_HTTP_Response* req = static_cast<JS_HTTP_Response*>(JS_GetOpaque2(ctx, this_val, js_response_class_id));
    if(!req)
        return JS_ThrowInternalError(ctx, "[js_write] if(!req)");

    JSScope <10,10> scope(ctx);
    if(argc==2)
    {
        MUTEX_INSPECTOR;
        if(!JS_IsString(argv[0])) return JS_ThrowTypeError(ctx, "[js_set_header] if(!JS_IsString(ctx,argv[0]))");

        if(!JS_IsString(argv[1])) return JS_ThrowTypeError(ctx, "[js_set_header] if(!JS_IsString(ctx,argv[1]))");

        auto key=scope.toStdStringView(argv[0]);
        auto val=scope.toStdStringView(argv[1]);
        req->respP->resp.setHeader(std::string( key),std::string(val));

    }
    else if(argc==1)
    {
        MUTEX_INSPECTOR;
        if(!JS_IsObject(argv[0])) return JS_ThrowTypeError(ctx, "[js_set_header] if(!JS_IsObject(ctx,argv[0]))");
        // JSValue obj=argv[0];
        JSPropertyEnum *tab;
        uint32_t len;
        int ret=JS_GetOwnPropertyNames(ctx,&tab,&len,argv[0],JS_GPN_STRING_MASK|JS_GPN_ENUM_ONLY);
        for(int i=0; i<len; i++)
        {
            size_t key_len;
            const char * _key=JS_AtomToCString(ctx,tab[i].atom);
            if(!_key)
            {
                return JS_EXCEPTION;
            }
            std::string key(_key);
            JS_FreeCString(ctx,_key);
            JSValue val=JS_GetProperty(ctx,argv[0],tab[i].atom);
            scope.addValue(val);
            if(JS_IsString(val))
            {
                size_t val_len;
                auto val_str=scope.toStdStringView(val);
                req->respP->resp.setHeader(std::string(key),std::string(val_str));
            }
        }
        js_free(ctx, tab);
    }
    else
        return JS_ThrowSyntaxError(ctx, "[js_set_header] number of argument must be 1 (jsobject) or 2 (string,string)");

    // return JS_DupValue(ctx, this_val);
    return JS_UNDEFINED;
}
static JSValue js_end(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    MUTEX_INSPECTOR;

    JS_HTTP_Response* req = static_cast<JS_HTTP_Response*>(JS_GetOpaque2(ctx, this_val, js_response_class_id));
    if(!req) return JS_ThrowInternalError(ctx, "[js_end] if(!req)");

    req->status_set_allowed=false;

    JSScope <10,10> scope(ctx);

    bool chunked=req->respP->resp.is_chunked;

    if(req->respP->resp.request->esi->closed())
    {
        return JS_ThrowReferenceError(ctx, "[end] attempt write to  closed socket");
    }

    std::string bufz;
    for(int i=0; i<argc; i++)
    {
        if(JS_IsString(argv[i]))
        {
            auto s=scope.toStdStringView(argv[i]);
            bufz+=s;

        }
        else if(JS_IsObject(argv[i]))
        {

            std::string buf;
            qjs::convert_js_value_to_json(ctx, argv[i],buf);

            bufz+=buf;

        }
        else
        {
            return JS_ThrowTypeError(ctx, "[js_send] unsupported arg type i=%d",i);
        }

    }
    if(chunked)
    {
        if(bufz.size())
        {
            req->respP->resp.send_chunked(bufz);
        }
        req->respP->resp.end_chunked();
    }
    else
    {
        if(bufz.size())
        {
            req->respP->resp.make_response(bufz);
        }
    }

    return JS_UNDEFINED;
}

static JSValue js_send(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    MUTEX_INSPECTOR;
    JS_HTTP_Response* req = static_cast<JS_HTTP_Response*>(JS_GetOpaque2(ctx, this_val, js_response_class_id));
    if(!req) return JS_ThrowInternalError(ctx, "[js_write] if(!req)");


    if(req->respP->resp.request->esi->closed())
    {
        return JS_ThrowReferenceError(ctx, "[send] attempt write to  closed socket");
    }

    req->status_set_allowed=false;
    JSScope <10,10> scope(ctx);
    std::string bufz;
    for(int i=0; i<argc; i++)
    {
        std::string buf;
        if(JS_IsString(argv[i]))
        {
            auto s=scope.toStdStringView(argv[i]);
            bufz+=s;
        }
        else if(JS_IsObject(argv[i]))
        {


            std::string buf;
            qjs::convert_js_value_to_json(ctx, argv[i],buf);
            bufz+=buf;
        }
        else
        {
            return JS_ThrowTypeError(ctx, "[js_send] unsupported arg type i=%d",i);
        }

    }
    req->respP->resp.send_chunked(bufz);
    return JS_UNDEFINED;
}
static JSValue js_status(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    MUTEX_INSPECTOR;
    JS_HTTP_Response* req = static_cast<JS_HTTP_Response*>(JS_GetOpaque2(ctx, this_val, js_response_class_id));
    if(!req) return JS_ThrowInternalError(ctx, "[js_status] if(!req)");

    if(req->status_set_allowed==false) return JS_ThrowInternalError(ctx, "[js_status] if(req->status_set_allowed==false)");


    if(argc!=1) return JS_ThrowTypeError(ctx, "[js_status] number of argument must be 1");
    if(!JS_IsNumber(argv[0])) return JS_ThrowTypeError(ctx, "[js_status] if(!JS_IsNumber(ctx,argv[0]))");
    int status=0;
    if(JS_ToInt32(ctx,&status,argv[0])<0) return JS_ThrowTypeError(ctx, "[js_status] if(JS_ToInt32(ctx,&status,argv[0])<0)");
    req->respP->resp.setStatus(status);
    req->status_set_allowed=false;
    return JS_DupValue(ctx, this_val);
    return JS_UNDEFINED;
}

#include "quickjs.h"
#include <string_view>
#include <vector>


static const JSCFunctionListEntry js_response_proto_funcs[] = {
    JS_CFUNC_DEF("setHeader", 2, js_set_header),
    JS_CFUNC_DEF("write", 1, js_send),
    JS_CFUNC_DEF("send", 1, js_send),
    JS_CFUNC_DEF("status", 1, js_status),
    JS_CFUNC_DEF("end", 1, js_end),
};

void register_http_response_class(JSContext* ctx)
{
    MUTEX_INSPECTOR;
    JS_NewClassID(&js_response_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_response_class_id, &js_response_class);

    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_response_proto_funcs, sizeof(js_response_proto_funcs) / sizeof(JSCFunctionListEntry));
    JS_SetClassProto(ctx, js_response_class_id, proto);

}

JSValue js_http_response_new(JSContext *ctx, const REF_getter<HTTP_ResponseP>& resp)
{
    MUTEX_INSPECTOR;
    JS_HTTP_Response* rsp = new JS_HTTP_Response(resp);
    JSValue jsRsp = JS_NewObjectClass(ctx,::js_response_class_id);
    DBG(memctl_add_object(jsRsp, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

    qjs::checkForException(ctx,jsRsp,"RequestIncoming: JS_NewObjectClass");

    JS_SetOpaque(jsRsp, rsp);
    return jsRsp;
}
