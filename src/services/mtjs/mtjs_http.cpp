#include "mtjsService.h"
#include "epoll_socket_info.h"
#include "jslib/net/http/http_server_request.h"
#include "jslib/net/http/http_server_response.h"
#include "common/jsscope.h"
#include "malloc_debug.h"

bool MTJS::Service::RequestIncoming(const httpEvent::RequestIncoming* e)
{
    MUTEX_INSPECTOR;
    REF_getter<HTTP_ResponseP> resp=new HTTP_ResponseP(e->req);

    JSScope scope(js_ctx);

    if(!e->esi->server_name_.has_value())
        throw CommonError("if(!e->esi->server_name_.has_value())");
    try {



        auto it=rconf->servers.find(*e->esi->server_name_);

        if(it!=rconf->servers.end())
        {
            if(it->second->typ!=server_conf_base::TYPE_HTTP)
            {
                throw CommonError("if(it->second->typ!=server_conf_base::TYPE_HTTP)");
            }
            server_conf_http* serv_conf_http=dynamic_cast<server_conf_http*>(it->second.get());
            if(serv_conf_http==nullptr)
                throw CommonError("if(serv_conf_http==nullptr)");

            {

                JSValue jsReq=js_http_request_new(js_ctx, e->req);
                scope.addValue(jsReq);
                DBG(memctl_add_object(jsReq,"js_http_request_new"));



                JSValue jsRsp=js_http_response_new(js_ctx, resp);
                scope.addValue(jsRsp);
                DBG(memctl_add_object(jsRsp,"js_http_response_new"));


                JSValue argv[] = { jsReq,jsRsp};
                int argc=2;
                JSValue global_obj = JS_GetGlobalObject(js_ctx);
                scope.addValue(global_obj);

                if (!JS_IsFunction(js_ctx, serv_conf_http->callback)) {
                    throw CommonError("Ошибка: callback не является функцией");
                }
                bool args_valid = true;
                for (int i = 0; i < argc; i++) {
                    if (JS_IsUndefined(argv[i]) || JS_IsNull(argv[i])) {
                        args_valid = false;
                        throw CommonError("Ошибка: аргумент %d некорректен", i);
                    }
                }


                JSValue func_result = JS_Call(js_ctx, serv_conf_http->callback, global_obj, argc, argv);



                scope.addValue(func_result);
                DBG(memctl_add_object(func_result,"JS_Call"));


                qjs::checkForException(js_ctx,func_result,"RequestIncoming: JS_Call");

                if(e->req->chunked && !e->req->reader.valid())
                {
                    logErr2("request chunked - you must define input stream");
                }


            }
        }


    }
    catch(CommonError &err)
    {
        logErr2("error: %s",err.what());
        resp->resp.setStatus(413);
        resp->resp.setHeader("Error",err.what());
        resp->resp.make_response(err.what());
    }


    return true;
}


bool MTJS::Service::RequestChunkReceived(const httpEvent::RequestChunkReceived* e)
{
    MUTEX_INSPECTOR;

    JSScope scope(js_ctx);


    e->req->reader->write("data",e->buf);

    return true;
}
bool MTJS::Service::RequestStartChunking(const httpEvent::RequestStartChunking* e)
{

    return false;

}
bool MTJS::Service::RequestChunkingCompleted(const httpEvent::RequestChunkingCompleted* e)
{

    JSScope scope(js_ctx);
    if(!e->req->reader.valid())
        throw CommonError("chunked request, you must specify stream");

    e->req->reader->write("end","");

    return true;
}
