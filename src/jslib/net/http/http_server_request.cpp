#include <quickjs.h>
#include <string>
#include "httpConnection.h"
#include "http_server_request.h"
#include "common/js_tools.h"
#include "ioStreams.h"
#include "quickjs.h"
#include <string_view>
#include <vector>
#include "httpUrlParser.h"
static JSClassID js_request_class_id;

class JS_HTTP_Request {
public:
    REF_getter<HTTP::Request> req;


    JS_HTTP_Request(JSContext *ctx, const REF_getter<HTTP::Request>& r) : req(r)
    {
        DBG(iUtils->mem_add_ptr("JS_HTTP_Request",this));

        DBG(logErr2("JS_HTTP_Request()"));
    }
    ~JS_HTTP_Request()
    {
        DBG(iUtils->mem_remove_ptr("JS_HTTP_Request",this));
        DBG(logErr2("~JS_HTTP_Request()"));

    }

};


static void js_request_finalizer(JSRuntime* rt, JSValue val) {
    JS_HTTP_Request* req = static_cast<JS_HTTP_Request*>(JS_GetOpaque(val, js_request_class_id));
    if (req) {
        delete req;
    }
}

static JSClassDef js_request_class = {
    "Request",
    .finalizer = js_request_finalizer,
};



// Идентификатор класса, если ещё не определён
// extern JSClassID js_request_class_id;

// Геттер для свойства 'url'
static JSValue js_request_get_url(JSContext* ctx, JSValueConst this_val/*, int magic*/)
{
    MUTEX_INSPECTOR;
    JS_HTTP_Request* req = static_cast<JS_HTTP_Request*>(JS_GetOpaque2(ctx, this_val, js_request_class_id));
    if (!req) return JS_EXCEPTION;
    auto url=req->req->url();
    return JS_NewStringLen(ctx, url.data(),url.size());
}

static JSValue js_request_get_method(JSContext* ctx, JSValueConst this_val/*, int magic*/)
{
    MUTEX_INSPECTOR;
    JS_HTTP_Request* req = static_cast<JS_HTTP_Request*>(JS_GetOpaque2(ctx, this_val, js_request_class_id));
    if (!req) return JS_EXCEPTION;

    auto &m=req->req->parse_data.method;
    return JS_NewInt32(ctx, m);
}

static JSValue js_request_get_content(JSContext* ctx, JSValueConst this_val/*, int magic*/)
{

    MUTEX_INSPECTOR;
    JS_HTTP_Request* req = static_cast<JS_HTTP_Request*>(JS_GetOpaque2(ctx, this_val, js_request_class_id));
    if (!req) return JS_EXCEPTION;
    if(req->req->chunked)
        return JS_ThrowReferenceError(ctx, "cannot read content in chunked request");
    return JS_NewStringLen(ctx, req->req->post_content.data(),req->req->post_content.size());
}

static JSValue js_request_get_stream(JSContext* ctx, JSValueConst this_val/*, int magic*/)
{
    return JS_ThrowReferenceError(ctx, "cannot read stream value from httpRequst");
}

static JSValue js_request_set_stream(JSContext* ctx, JSValueConst this_val, JSValueConst val)
{
    MUTEX_INSPECTOR;

    JS_HTTP_Request* req = static_cast<JS_HTTP_Request*>(JS_GetOpaque2(ctx, this_val, js_request_class_id));
    if (!req) return JS_EXCEPTION;

    if(!js_IsStream(ctx,val))
    {
        return JS_ThrowReferenceError(ctx, "val must be stream");
    }
    if(!req->req->chunked)
        return JS_ThrowReferenceError(ctx, "stream not allowed because request is not chunked");

    if(req->req->reader.valid())
        return JS_ThrowReferenceError(ctx, "stream already assigned");

    req->req->reader=js_get_stream_CPP(ctx,val);

    return JS_UNDEFINED;
}

static JSValue js_is_chunked(JSContext* ctx, JSValueConst this_val/*, int magic*/)
{
    MUTEX_INSPECTOR;

    JS_HTTP_Request* req = static_cast<JS_HTTP_Request*>(JS_GetOpaque2(ctx, this_val, js_request_class_id));
    if (!req) return JS_EXCEPTION;
    return JS_NewBool(ctx,req->req->chunked);
}
static JSValue js_is_websocket(JSContext* ctx, JSValueConst this_val/*, int magic*/)
{
    MUTEX_INSPECTOR;

    JS_HTTP_Request* req = static_cast<JS_HTTP_Request*>(JS_GetOpaque2(ctx, this_val, js_request_class_id));
    if (!req) return JS_EXCEPTION;
    return JS_NewBool(ctx,req->req->isWebSocket);
}

static JSValue js_request_get_headers(JSContext* ctx, JSValueConst this_val/*, int magic*/)
{
    MUTEX_INSPECTOR;

    JS_HTTP_Request* req = static_cast<JS_HTTP_Request*>(JS_GetOpaque2(ctx, this_val, js_request_class_id));
    if (!req) return JS_EXCEPTION;
    JSValue js_row = JS_NewObject(ctx);
    DBG(memctl_add_object(js_row, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

    int i=0;
    // for(auto& z: req->req->headers)
    {
        // JS_DefinePropertyValueStr(ctx, js_row, z.first.c_str(), JS_NewStringLen(ctx,z.second.data(),z.second.size()), JS_PROP_C_W_E);
    }

    return js_row;
}

// Макросы для удобства с string_view в printf
//#define SV_FMT "%.*s"
//#define SV_ARG(sv) static_cast<int>((sv).size()), (sv).data()
#include "sv.h"

#include "quickjs.h"
#include "httpHeadersParser.h"
static JSValue js_parse_headers_simple(JSContext* ctx, JSValueConst this_val,
                                      int argc, JSValueConst* argv) {
    

    JS_HTTP_Request* req = static_cast<JS_HTTP_Request*>(JS_GetOpaque2(ctx, this_val, js_request_class_id));
    if (!req) return JS_EXCEPTION;


    auto headers=req->req->headers();

    
    SimpleHeadersParser parser;
    parser.parse(headers);
    
    // Создаем объект
    JSValue headers_obj = JS_NewObject(ctx);
    
    // Заполняем заголовки
    for (const auto& [key, value] : parser.headers()) {
        JSAtom atom = JS_NewAtomLen(ctx, key.data(),key.size());
        JSValue js_value = JS_NewStringLen(ctx, value.data(),value.size());
        JS_SetProperty(ctx, headers_obj, atom, js_value);
        JS_FreeAtom(ctx, atom);
    }
    
    return headers_obj;
}

static JSValue js_parse_uri_no_malloc(JSContext* ctx, JSValueConst this_val,
                                     int argc, JSValueConst* argv) {


    JS_HTTP_Request* req = static_cast<JS_HTTP_Request*>(JS_GetOpaque2(ctx, this_val, js_request_class_id));
    if (!req) return JS_EXCEPTION;


    auto url=req->req->url();
    
    // const char* url_cstr = JS_ToCString(ctx, argv[0]);
    // if (!url_cstr) {
    //     return JS_EXCEPTION;
    // }
    
    // std::string_view url(url_cstr);
    URIParser parser;
    
    if (!parser.parse(url)) {
        return JS_ThrowTypeError(ctx, "Failed to parse URL");
    }
    
    // Создаем основной объект URL
    JSValue url_obj = JS_NewObject(ctx);
    
    // Создаем объект для параметров
    JSValue params_obj = JS_NewObject(ctx);
    
    // Заполняем параметры
    for (const auto& [key, value] : parser.params()) {
        JSAtom atom_key = JS_NewAtomLen(ctx, key.data(), key.size());
        // JSValue js_key = JS_NewStringLen(ctx, key.data(), key.size());
        JSValue js_value = JS_NewStringLen(ctx, value.data(), value.size());
        JS_SetProperty(ctx, params_obj, atom_key, js_value);
        JS_FreeAtom(ctx, atom_key);
        // JS_FreeValue(ctx, js_value);
    }
    
    // Заполняем свойства основного объекта
    JS_SetPropertyStr(ctx, url_obj, "full", 
                     JS_NewStringLen(ctx, parser.full().data(), parser.full().size()));
    JS_SetPropertyStr(ctx, url_obj, "path", 
                     JS_NewStringLen(ctx, parser.path().data(), parser.path().size()));
    JS_SetPropertyStr(ctx, url_obj, "query", 
                     JS_NewStringLen(ctx, parser.query().data(), parser.query().size()));
    JS_SetPropertyStr(ctx, url_obj, "fragment", 
                     JS_NewStringLen(ctx, parser.fragment().data(), parser.fragment().size()));
    JS_SetPropertyStr(ctx, url_obj, "params", params_obj);
    
    return url_obj;
}

static const JSCFunctionListEntry js_request_proto_funcs[] = {

    JS_CFUNC_DEF("parseURI", 0, js_parse_uri_no_malloc),
    JS_CFUNC_DEF("parseHeaders", 0, js_parse_headers_simple),

    // JS_CGETSET_DEF("url", js_request_get_url, NULL),
    JS_CGETSET_DEF("method", js_request_get_method, NULL),
    JS_CGETSET_DEF("content", js_request_get_content, NULL),
    // JS_CGETSET_DEF("headers", js_request_get_headers, NULL),
    JS_CGETSET_DEF("stream",  js_request_get_stream,js_request_set_stream),
    JS_CGETSET_DEF("is_chunked",  js_is_chunked,NULL),
    JS_CGETSET_DEF("is_websocket",  js_is_websocket,NULL),
};

void register_http_request_class(JSContext* ctx)
{
    MUTEX_INSPECTOR;
    JS_NewClassID(&js_request_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_request_class_id, &js_request_class);

    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_request_proto_funcs, sizeof(js_request_proto_funcs) / sizeof(JSCFunctionListEntry));

    JS_SetClassProto(ctx, js_request_class_id, proto);

}


JSValue js_http_request_new(JSContext *ctx, const REF_getter<HTTP::Request>& request)
{
    MUTEX_INSPECTOR;

    JS_HTTP_Request* req = new JS_HTTP_Request(ctx, request);
    JSValue jsReq = JS_NewObjectClass(ctx,::js_request_class_id);
    DBG(memctl_add_object(jsReq, (std::string(__FILE__+std::to_string(__LINE__))).c_str()));

    qjs::checkForException(ctx,jsReq,"RequestIncoming: JS_NewObjectClass");
    JS_SetOpaque(jsReq, req);
    return jsReq;
}
