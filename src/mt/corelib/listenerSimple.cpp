#include "listenerSimple.h"
#include "IUtils.h"
#include "mutexInspector.h"
#include "commonError.h"
ListenerSimple::~ListenerSimple()
{
}


ListenerSimple::ListenerSimple(const std::string& name, const SERVICE_id& sid):ListenerBase(name,sid) {}


void ListenerSimple::listenToEvent(const REF_getter<Event::Base>& e)
{
    XTRY;
    try {
        if(!e.valid()) return;
        if(!handleEvent(e.get()))
        {
            logErr2("ListenerSimple: unhandled event %s  in %s",iUtils->genum_name(e->id),listenerName_.c_str());
        }
    }
    catch(const CommonError& ec)
    {
        logErr2("CommonError: %s %s",ec.what(),_DMI().c_str());

    }
    catch(const std::exception &ec)
    {
        logErr2("ListenerSimple std::exception: %s",ec.what());
    }
    XPASS;
}

