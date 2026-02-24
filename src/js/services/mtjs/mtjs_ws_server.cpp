#include "mtjsService.h"
extern JSClassID js_ws_server_connection_class_id;
bool MTJS::Service::WSDisaccepted(const httpEvent::WSDisaccepted*e)
{
    MUTEX_INSPECTOR;
    JSScope <10,10> scope(js_ctx);
    e->req->ctx->getReader()->write("disaccepted",NULL,0);

    return true;
}
bool MTJS::Service::WSDisconnected(const httpEvent::WSDisconnected*e)
{
    e->req->getReader()->write("disconnected",NULL,0);

    return false;
}
bool MTJS::Service::WSTextMessage(const httpEvent::WSTextMessage*e)
{
    MUTEX_INSPECTOR;
    e->req->getReader()->write("message",e->msg.data(),e->msg.size());
    return true;
}
