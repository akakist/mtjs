#include "mtjsService.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "common/timers.h"
#include "Events/Tools/webHandlerEvent.h"
extern "C"
{
}


void register_http_response_class(JSContext* ctx);
void register_http_request_class(JSContext* ctx) ;
void register_http_server_class(JSContext* ctx) ;
void register_ws_server_connection_class(JSContext* ctx);
void js_register_pipe_reader( JSContext *ctx,JSValue & mtjs_obj);
int js_init_module_mysql(JSContext *ctx) ;
void init_rpc_object(JSContext *ctx,JSValue & mtjs_obj);
void register_rpc_request_class(JSContext* ctx);
void register_rpc_response_class(JSContext* ctx);
void js_register_async_tools( JSContext *ct,JSValue & mtjs_obj);
void init_utils_object(JSContext *ctx,JSValue & mtjs_obj) ;
void init_telnet_object(JSContext *ctx,JSValue & mtjs_obj);

void js_init_fs(JSContext *ctx, JSValue mtjs_obj);




void init_timer(JSRuntime *rt, JSContext *gCtx);
int js_init_stdin_reader_global(JSContext *ctx,JSValue & mtjs_obj);
JSModuleDef *js_init_module_stream(JSContext *ctx, const char *module_name);
int js_init_stream_global(JSContext *ctx,JSValue & mtjs_obj);


extern "C"
{
    void register_functions_parse_json(JSContext* ctx);
    JSModuleDef* js_init_module_std(JSContext* ctx, const char* module_name);
    JSModuleDef* js_init_module_os(JSContext* ctx, const char* module_name);
    void add_mt_http_module(JSContext* ctx,JSValue & mtjs_obj);

#ifdef __linux__
#endif
}


extern "C"
void js_std_add_helpers(JSContext* ctx, int argc, char** argv);


// Функция для загрузки JS-кода из файла
std::string loadFile(const std::string& filename)
{
    std::ifstream file(filename);
    if(!file) throw std::runtime_error("Cannot open file: " + filename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

#include "quickjs-libc.h"

bool MTJS::Service::on_startService(const systemEvent::startService*)
{
    js_rt=JS_NewRuntime();
    if(!js_rt) throw CommonError("if(!js_rt)");
    js_std_init_handlers(js_rt);

    JS_SetMaxStackSize(js_rt, 48 * 1024 * 1024);

    js_ctx=JS_NewContext(js_rt);
    if(!js_ctx) throw CommonError("if(!js_ctx)");

    JSScope scope(js_ctx);
    JSValue mtjs_obj = JS_NewObject(js_ctx);

    js_init_module_std(js_ctx, "std");
    js_init_module_os(js_ctx, "os");
    add_mt_http_module(js_ctx,mtjs_obj);
    register_http_request_class(js_ctx);
    register_http_response_class(js_ctx);
    register_http_server_class(js_ctx);
    register_ws_server_connection_class(js_ctx);
    js_std_add_helpers(js_ctx,0,NULL);
    init_timer(js_rt, js_ctx);
    js_register_pipe_reader(js_ctx,mtjs_obj);
    js_init_stdin_reader_global(js_ctx,mtjs_obj);
    js_init_stream_global(js_ctx,mtjs_obj);

    js_init_module_mysql(js_ctx);
    init_rpc_object(js_ctx,mtjs_obj);
    register_rpc_request_class(js_ctx);
    register_rpc_response_class(js_ctx);
    js_register_async_tools(js_ctx,mtjs_obj);
    js_init_fs(js_ctx, mtjs_obj);


    init_utils_object(js_ctx,mtjs_obj);
    init_telnet_object(js_ctx,mtjs_obj);

    auto global_object=JS_GetGlobalObject(js_ctx);
    JS_SetPropertyStr(js_ctx, global_object, "mtjs", mtjs_obj);
    JS_FreeValue(js_ctx,global_object);









    rconf=new RCF(js_ctx);



    js_std_add_helpers(js_ctx, 0, NULL);

    opaque.broadcaster = this;
    opaque.rcf = rconf;
    opaque.listener = this;

    JS_SetContextOpaque(js_ctx, &opaque);

    sendEvent(ServiceEnum::Timer, new timerEvent::SetTimer(Timers::TIMER_POLL,NULL,NULL,pending_timeout,this));
#ifdef WEBDUMP
    sendEvent(ServiceEnum::WebHandler, new webHandlerEvent::RegisterDirectory("mem","Memleaks"));

    sendEvent(ServiceEnum::WebHandler,new webHandlerEvent::RegisterHandler("mem/info","MemLeaks",this));
#endif
    return true;
}
