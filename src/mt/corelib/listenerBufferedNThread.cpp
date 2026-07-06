#include "listenerBufferedNThread.h"
#include "IUtils.h"
#include "IInstance.h"
#include "mutexInspector.h"
#include "colorOutput.h"
#include "commonError.h"
ListenerBufferedNThread::~ListenerBufferedNThread()
{
}
void ListenerBufferedNThread::deinit()
{
    auto cont=m_container;
    m_container=NULL;
    cont->clear();
    cont->push(NULL);
    if(m_pt.size())
    {
        m_isTerminating=true;
        cont->deinit();
        cont->m_cond.broadcast();
        for(auto& z: m_pt)
        {
            int err=pthread_join(z,NULL);
            if(err)
            {
                printf(RED("%s pthread_join: %s"),__PRETTY_FUNCTION__,strerror(errno));
            }
        }

    }

}
ListenerBufferedNThread::ListenerBufferedNThread(IInstance *ins,const std::string& name, const SERVICE_id & sid,size_t stackSize)
    :ListenerBase(name,sid),m_container(new EventDeque),m_pt(0),m_isTerminating(false) {

    n_threads=ins->getConfig()->get_int64_t("n_threads",20,"number of workers");
    for(int i=0; i<n_threads; i++)
    {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, stackSize);

        XTRY;
        pthread_t pt;
        if(pthread_create(&pt,&attr,worker,this))
            throw CommonError("pthread_create: errno %d",errno);

        m_pt.push_back(pt);
        XPASS;
    }

}

void ListenerBufferedNThread::processEvent(const REF_getter<Event::Base>&e)
{
    try {
        if(!handleEvent(e))
        {
            XTRY;
            logErr2("ListenerBufferedNThread: unhandled event %s svs=%s in listener=%s",
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
        logErr2("ListenerBufferedNThread std::exception: " RED2("%s %s"),e.what(),_DMI().c_str());
    }

}

void* ListenerBufferedNThread::worker(void*p)
{
#ifdef __MACH__
    pthread_setname_np("ListenerBufferedNThread");
#else
#if !defined _WIN32 && !defined __FreeBSD__

    pthread_setname_np(pthread_self(),"ListenerBufferedNThread");
#endif
#endif
    ListenerBufferedNThread* l=static_cast<ListenerBufferedNThread*>(p);
    while(1)
    {
        try
        {
            if(l->m_isTerminating)
            {
                DBG(printf(BLUE("ListenerBufferedNThread exit %s"),l->listenerName_.c_str()));
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
                    DBG(printf(BLUE("ListenerBufferedNThread exit %s"),l->listenerName_.c_str()));
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


void ListenerBufferedNThread::listenToEvent(const REF_getter<Event::Base>& e)
{

    if(!m_container.valid())
    {
        logErr2("if(!m_container.valid())");
        return;
    }
    XTRY;
    m_container->push(e);
    XPASS;
}
