#include "mtjsService.h"
#include "epoll_socket_info.h"
#include "jslib/net/http/http_server_request.h"
#include "jslib/net/http/http_server_response.h"
#include "common/jsscope.h"


bool MTJS::Service::RequestIncoming(const httpEvent::RequestIncoming* e)
{
    MUTEX_INSPECTOR;
    REF_getter<HTTP_ResponseP> resp=new HTTP_ResponseP(e->req);

    JSScope <10,10> scope(js_ctx);

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



                JSValue jsRsp=js_http_response_new(js_ctx, resp);
                scope.addValue(jsRsp);


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
                
                qjs::checkForException(js_ctx,func_result,"RequestIncoming: JS_Call");

                if(e->req->is_chunked && !e->req->getReader().valid())
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
    // logErr2("RequestChunkReceived chunkId %lu, buf size %lu",e->chunkId,e->buf.size());
    // JSScope <10,10> scope(js_ctx);

    // logErr2("writing chunk to stream %p",e->req->getReader().get());
    if(!e->req->getReader().valid())
    {
        throw CommonError("stream not defined for chunked request");
        return false;
    }

    e->req->getReader()->write("data",e->buf->container.data(),e->buf->container.size());

    return true;
}
bool MTJS::Service::RequestStartChunking(const httpEvent::RequestStartChunking* e)
{

    logErr2("RequestStartChunking %s",__func__);
    return true;

}
bool MTJS::Service::RequestChunkingCompleted(const httpEvent::RequestChunkingCompleted* e)
{

    // JSScope<10,10> scope(js_ctx);
    if(!e->req->getReader().valid())
        throw CommonError("chunked request, you must specify stream");

    e->req->getReader()->write("end",NULL,0);

    return true;
}
