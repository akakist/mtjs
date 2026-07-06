#pragma once
#include "listenerBase.h"
#include "eventDeque.h"
class ListenerBuffered1Thread;
/**
*       Single thread listener, used if you need to create service without internal multithreading, i.e. without mutexes.
*       Each ListenerBuffered1Thread has own thread
*/
class ListenerBuffered1Thread:public ListenerBase
{

    REF_getter<EventDeque> m_container;
    pthread_t m_pt;
    bool m_isTerminating;
    static void* worker(void*);
protected:

    /// implementation of base virtual
    void listenToEvent(const REF_getter<Event::Base>&e) final;
    size_t getPendingCount() const final
    {
        return m_container->size();
    }

public:
    void deinit();

    ListenerBuffered1Thread(const std::string &name, const SERVICE_id &sid,size_t stacksize=(6*1024*1024));

    /// call method in inherited class to process event
    virtual bool handleEvent(const REF_getter<Event::Base>& e)=0;

    virtual ~ListenerBuffered1Thread();

    /// caller of handleEvent
    void processEvent(const REF_getter<Event::Base>&);



};
