#include "broadcaster.h"
#include "mutexInspector.h"
#include "commonError.h"
void Broadcaster::sendEvent(const SERVICE_id& svs, const REF_getter<Event::Base>&e)
{
    XTRY;
    __BRDifa->sendEvent(svs,e);
    XPASS;
}
void Broadcaster::sendEvent( ListenerBase* svs, const REF_getter<Event::Base>&e)
{
    XTRY;
    __BRDifa->sendEvent(svs,e);
    XPASS;
}
void Broadcaster::sendEvent(const msockaddr_in& addr, const SERVICE_id& svs, const REF_getter<Event::Base>&e)
{
    XTRY;
    __BRDifa->sendEvent(addr,svs,e);
    XPASS;
}
void Broadcaster::sendEvent(const std::string& addr, const SERVICE_id& svs, const REF_getter<Event::Base>&e)
{
    XTRY;
    __BRDifa->sendEvent(addr,svs,e);
    XPASS;
}

void Broadcaster::passEvent(const REF_getter<Event::Base>&e)
{
    XTRY;
    __BRDifa->passEvent(e);
    XPASS;
}
