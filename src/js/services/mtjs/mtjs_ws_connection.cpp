#include "mtjsService.h"

bool MTJS::Service::StreamRead(const socketEvent::StreamRead*e)
{
    logErr2("-->StreamRead sockId %ld",CONTAINER(e->esi->id_));

    return true;
}
bool MTJS::Service::Connected(const socketEvent::Connected*e)
{

    return true;
}

bool MTJS::Service::Disconnected(const socketEvent::Disconnected*e)
{
    return true;
}
