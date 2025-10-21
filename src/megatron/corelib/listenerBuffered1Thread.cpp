#include "listenerBuffered1Thread.h"
#include "IUtils.h"
#include "mutexInspector.h"
#include "colorOutput.h"
#include "commonError.h"
ListenerBuffered1Thread::~ListenerBuffered1Thread()
{
}
void ListenerBuffered1Thread::deinit()
{
    auto cont=m_container;
    m_container=NULL;
    cont->clear();
    cont->push(NULL);
    if(m_pt)
    {
        m_isTerminating=true;
        cont->deinit();
        cont->m_cond.broadcast();
        int err=pthread_join(m_pt,NULL);
        if(err)
        {
            printf(RED("%s pthread_join: %s"),__PRETTY_FUNCTION__,strerror(errno));
        }

    }

}
ListenerBuffered1Thread::ListenerBuffered1Thread(const std::string& name, const SERVICE_id & sid,size_t stackSize)
    :ListenerBase(name,sid),m_container(new EventDeque),m_pt(0),m_isTerminating(false) {

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, stackSize);
    XTRY;
    if(pthread_create(&m_pt,&attr,worker,this))
        throw CommonError("pthread_create: errno %d",errno);
    XPASS;

}

void ListenerBuffered1Thread::processEvent(const REF_getter<Event::Base>&e)
{
    try {
        if(!handleEvent(e))
        {
            XTRY;
            logErr2("ListenerBuffered1Thread: unhandled event %s svs=%s in listener=%s",
                    iUtils->genum_name(e->id),listenerName_.c_str(), listenerName_.c_str());
            XPASS;

        }
    }
    catch(const CommonError& e)
    {
        logErr2("CommonError: " RED2("%s %s"),e.what(),_DMI().c_str());

    }
    catch(const std::exception &e)
    {
        logErr2("ListenerBuffered1Thread std::exception: " RED2("%s %s"),e.what(),_DMI().c_str());
    }

}

void* ListenerBuffered1Thread::worker(void*p)
{
#ifdef __MACH__
    pthread_setname_np("ListenerBuffered1Thread");
#else
#if !defined _WIN32 && !defined __FreeBSD__

    pthread_setname_np(pthread_self(),"ListenerBuffered1Thread");
#endif
#endif
    ListenerBuffered1Thread* l=static_cast<ListenerBuffered1Thread*>(p);
    while(1)
    {
        try
        {
            if(l->m_isTerminating)
            {
                DBG(printf(BLUE("ListenerBuffered1Thread exit %s"),l->listenerName_.c_str()));
                return NULL;
            }
            EventDeque* dq=l->m_container.get();
            if(dq==nullptr)
            {
                logErr2("!dq valid");
                return NULL;
            }

            auto e=dq->pop();
            for(auto z:e)
            {
                if(z.valid())
                {
                    l->processEvent(z);
                }
                else
                {
                    DBG(printf(BLUE("ListenerBuffered1Thread exit %s"),l->listenerName_.c_str()));
                    return NULL;
                }

            }

        }
        catch(const std::exception &e)
        {
            logErr2("exception: " RED2("%s") " (%s) %s",e.what(),l->listenerName_.c_str(),_DMI().c_str());
        }
    }
    return NULL;
}


void ListenerBuffered1Thread::listenToEvent(const REF_getter<Event::Base>& e)
{

    if(!m_container.valid())
    {
        // logErr2("if(!m_container.valid()) %s",listenerName_.c_str());
        return;
    }
    XTRY;
    m_container->push(e);
    XPASS;
}
