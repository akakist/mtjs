#pragma once

#include "quickjs.h"
#include "listenerBase.h"
#include "mutexable.h"
#include "mutexInspector.h"
#include "commonError.h"
struct async_task: public Refcountable
{
    ListenerBase* listener;

    /// @brief  this func must run in worker thread
    virtual void execute()=0;

    /// this func must run in MTJS thread
    /// call resolve or reject
    /// execute and finalize executes in another threads and class members must be locked by mutexes
    virtual void finalize(JSContext *ctx)=0;
    async_task(ListenerBase* l): listener(l) {
        DBG(iUtils->mem_add_ptr("async_task",this));

    }
    virtual ~async_task()
    {
        DBG(iUtils->mem_remove_ptr("async_task",this));
    }

};

struct op_deque: public Refcountable
{
    MutexC m_mutex;
    Condition m_cond;
    bool m_isTerminating;
    std::deque<REF_getter<async_task> > container;
    int counter=0;

    op_deque()
        :
        m_cond(m_mutex),
        m_isTerminating(false) {
        DBG(iUtils->mem_add_ptr("op_deque",this));
    }
    ~op_deque()
    {
        DBG(iUtils->mem_remove_ptr("op_deque",this));
        stop();

    }
    void deinit()
    {
        MUTEX_INSPECTOR;
        m_isTerminating=true;
        m_cond.broadcast();
    }

    bool empty()
    {
        M_LOCKC(m_mutex);
        return container.size()==0 && counter==0;

    }
    void push(const REF_getter<async_task> & e)
    {
        MUTEX_INSPECTOR;
        {
            M_LOCKC(m_mutex);
            container.push_back(e);
        }
        m_cond.signal();
    }
    REF_getter<async_task>  pop()
    {
        MUTEX_INSPECTOR;
        M_LOCKC(m_mutex);
        while(1)
        {
            if(m_isTerminating)
                return nullptr;
            if(!container.size())
            {
                m_cond.wait();
            }
            if(m_isTerminating)
                return nullptr;
            if(container.size())
            {
                REF_getter<async_task>  r=container[0];
                container.pop_front();
                counter++;
                return r;
            }
        }
    }
    void decCounter()
    {
        MUTEX_INSPECTOR;
        M_LOCKC(m_mutex);
        counter--;
    }

    void stop()
    {
        MUTEX_INSPECTOR;
        m_isTerminating=true;
        m_cond.broadcast();
    }





};
