#include "eventDeque.h"


void EventDeque::push(const REF_getter<Event::Base> & e)
{
    {
        M_LOCKC(m_mutex);
        container.push_back(e);
    }
    m_cond.signal();
}
std::deque<REF_getter<Event::Base> > EventDeque::pop()
{
    M_LOCKC(m_mutex);
    while(1)
    {
        if(m_isTerminating)
            return {};
        if(!container.size())
            m_cond.wait();
        if(m_isTerminating)
            return {};
        if(container.size())
        {
            std::deque<REF_getter<Event::Base> > r=std::move(container);
            container.clear();
            return r;
        }
    }
}
