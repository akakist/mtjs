#pragma once
#include "event_mt.h"

typedef bool (*eventhandler) (const REF_getter<Event::Base> &, void*);

/**
* Base listener class
*/

class ListenerBase
{
protected:
public:

    /// declaration, implementation in inherited
    virtual void listenToEvent(const REF_getter<Event::Base>&)=0;
    virtual size_t getPendingCount() const =0;

    /// name of listener
    const std::string listenerName_;


    ListenerBase(const std::string& nm, const SERVICE_id& sid);
    virtual ~ListenerBase() {}

    const SERVICE_id serviceId;
};


