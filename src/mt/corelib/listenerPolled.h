#pragma once
#include "listenerBase.h"
#include "mutexable.h"
class ListenerPolled;

///       Polled listener used mainly in GUI windows

class ListenerPolled:public ListenerBase
{
    Mutex lk;
    std::deque<REF_getter<Event::Base> > m_container_;

protected:

    /// implementation of base virtuals
    void listenToEvent(const REF_getter<Event::Base>&e) final;
    virtual size_t getPendingCount() const
    {
        M_LOCK(lk);
        return m_container_.size();
    }


public:
    /// call method in inherited class to process event
    virtual bool handleEvent(const REF_getter<Event::Base>& e)=0;

    virtual ~ListenerPolled();
    ListenerPolled(const std::string &name, const SERVICE_id &sid);

    /// GUI class must call this method in onTimer
    void poll();

    void deinit();

};
