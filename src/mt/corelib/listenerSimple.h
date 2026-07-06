#pragma once
#include "listenerBase.h"


/// simple listener w/o buffering and threads

class ListenerSimple:public ListenerBase
{
public:

    /// call method in inherited class to process event
    virtual bool handleEvent(const REF_getter<Event::Base>& e)=0;
    virtual ~ListenerSimple();
    ListenerSimple(const std::string &name, const SERVICE_id &sid);
    size_t getPendingCount() const final {
        return 0;
    }

    void deinit() {}

    /// implementation of base virtuals
    void listenToEvent(const REF_getter<Event::Base>& e) final;

};

