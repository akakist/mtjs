#include "listenerPolled.h"
#include "IUtils.h"
#include "mutexInspector.h"
#include "colorOutput.h"
#include "commonError.h"


ListenerPolled::~ListenerPolled()
{

    m_container_.clear();
}
ListenerPolled::ListenerPolled(const std::string& name, const SERVICE_id& sid)
    :ListenerBase(name,sid)
{
}

void ListenerPolled::poll()
{

    try
    {
        std::deque<REF_getter<Event::Base> > d;
        {
            M_LOCK(lk);
            d=std::move(m_container_);
            m_container_.clear();
        }
        for(size_t n=0; n<d.size(); n++)
        {
            try {
                if(!handleEvent(d[n]))
                {
                    XTRY;
                    logErr2("ListenerPolled: unhandled event %s in %s",iUtils->genum_name(d[n]->id),listenerName_.c_str());
                    XPASS;

                }
            }
            catch(const std::exception& e)
            {
                logErr2("ListenerPolled exception: " RED2("%s") " %s %s",e.what(),_DMI().c_str(),iUtils->genum_name(d[n]->id));

            }
        }
    }
    catch(const std::exception &e)
    {
        logErr2("ListenerPolled exception: %s %s ",e.what(),_DMI().c_str());
    }

    return;
}

void ListenerPolled::listenToEvent(const REF_getter<Event::Base>& e)
{

    XTRY;

    M_LOCK(lk);
    m_container_.push_back(e);
    XPASS;


}
void ListenerPolled::deinit()
{

}
