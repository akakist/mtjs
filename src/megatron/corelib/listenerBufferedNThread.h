#pragma once
#include "listenerBase.h"
#include "eventDeque.h"
#include "IInstance.h"
class ListenerBufferedNThread;
/**
*       Single thread listener, used if you need to create service without internal multithreading, i.e. without mutexes.
*       Each ListenerBufferedNThread has own thread
*/
class ListenerBufferedNThread:public ListenerBase
{

    REF_getter<EventDeque> m_container;
    int n_threads;
    std::vector<pthread_t> m_pt;
    bool m_isTerminating;
    static void* worker(void*);
protected:

    /// implementation of base virtual
    void listenToEvent(const REF_getter<Event::Base>&e) final;
public:
    void deinit();

    ListenerBufferedNThread(IInstance *ins,const std::string &name, const SERVICE_id &sid,size_t stacksize=(6*1024*1024));

    /// call method in inherited class to process event
    virtual bool handleEvent(const REF_getter<Event::Base>& e)=0;

    virtual ~ListenerBufferedNThread();

    /// caller of handleEvent
    void processEvent(const REF_getter<Event::Base>&);



};
