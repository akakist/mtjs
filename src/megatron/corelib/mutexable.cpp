#include "mutexable.h"
#include "IUtils.h"
#include <unistd.h>
#include "commonError.h"

Mutex::Mutex()
{
#ifdef _WIN32
    InitializeCriticalSection(&m_lock);
#else
    pthread_mutex_init(&m_lock, NULL);
#endif
}
MutexC::MutexC()
{
    pthread_mutex_init(&m_lock, NULL);
}

void Mutex::lock() const
{
    XTRY;
#ifdef _WIN32
    EnterCriticalSection((CRITICAL_SECTION*)&m_lock);
#else

#ifdef DEBUG
    time_t start=time(NULL);
    while(1)
    {
        if(pthread_mutex_trylock((pthread_mutex_t*)&m_lock))
        {

            if(time(NULL)-start>10)
            {
                throw CommonError("mutexlock timeout %s",iUtils->getIThreadNameController()->dump_mutex_inspectors().c_str());
            }
            usleep(1000);
            continue;

        }
        else return;
    }

#else
    if(pthread_mutex_lock((pthread_mutex_t*)&m_lock))
        throw CommonError("pthread_mutex_lock: errno %d", errno);
#endif
#endif
    XPASS;
}
void MutexC::lock() const
{
    XTRY;

#ifdef DEBUG
    time_t start=time(NULL);
    while(1)
    {
        if(pthread_mutex_trylock((pthread_mutex_t*)&m_lock))
        {

            if(time(NULL)-start>10)
            {
                throw CommonError("mutexlock timeout %s",iUtils->getIThreadNameController()->dump_mutex_inspectors().c_str());
            }
            usleep(1000);
            continue;

        }
        else return;
    }

#else
    if(pthread_mutex_lock((pthread_mutex_t*)&m_lock))
        throw CommonError("pthread_mutex_lock: errno %d", errno);
#endif
    XPASS;
}

void Mutex::unlock() const
{
#ifdef _WIN32
    LeaveCriticalSection((CRITICAL_SECTION*)&m_lock);
#else
    int r=pthread_mutex_unlock((pthread_mutex_t*)&m_lock);
#ifndef __FreeBSD__
    if(r)
        throw CommonError("pthread_mutex_unlock: errno %d %s r=%d", errno,strerror(errno),r);
#endif
#endif
}
void MutexC::unlock() const
{
    int r=pthread_mutex_unlock((pthread_mutex_t*)&m_lock);
    if(r)
        throw CommonError("pthread_mutex_unlock: errno %d %s", errno,strerror(errno));
}
Mutex::~Mutex()
{
#ifdef _WIN32
    DeleteCriticalSection(&m_lock);
#else
    if(pthread_mutex_destroy(&m_lock))
    {
        //logErr2("pthread_mutex_destroy: errno %d", errno);
    }
#endif
}
MutexC::~MutexC()
{
    if(pthread_mutex_destroy(&m_lock))
    {
        //logErr2("pthread_mutex_destroy: errno %d", errno);
    }
}

Condition::Condition(const MutexC&m): m_mx(m)
{
    XTRY;
    if(pthread_cond_init(&m_cond,NULL))
        throw CommonError("pthread_cond_init: errno %d", errno);
    XPASS;
};
void Condition::wait() const
{
    XTRY;
    if(pthread_cond_wait((pthread_cond_t*)&m_cond,(pthread_mutex_t*)&m_mx.m_lock))
        throw CommonError("pthread_cond_wait: errno %d", errno);
    XPASS;
}
void Condition::timedwait(const mtimespec& ts) const
{
    XTRY;
    int r=pthread_cond_timedwait((pthread_cond_t*)&m_cond,(pthread_mutex_t*)&m_mx.m_lock,&ts);

    if(r!=ETIMEDOUT && r!=EAGAIN && r!=ENOTSOCK)
    {
#ifndef _WIN32
        if(errno!=ETIMEDOUT && errno!=0 && errno!=EAGAIN && errno!=ENOTSOCK)
        {
        }
#else
        if(errno!=0)
        {
            //int eno=errno;

        }
#endif
    }
    XPASS;
}
void Condition::signal() const
{
    XTRY;
    if(pthread_cond_signal((pthread_cond_t*)&m_cond))
        throw CommonError("pthread_cond_signal: errno %d", errno);
    XPASS;
}
void Condition::broadcast() const
{
    XTRY;
    if(pthread_cond_broadcast((pthread_cond_t*)&m_cond))
        throw CommonError("pthread_cond_broadcast: errno %d", errno);
    XPASS;
}
Condition::~Condition()
{
    if(pthread_cond_destroy((pthread_cond_t*)&m_cond))
        logErr2("pthread_cond_destroy: errno %d", errno);
}
void RWLock::wrlock() const
{
    int err=pthread_rwlock_wrlock(&lock_rw);
    if(err!=0)
        throw CommonError("pthread_rwlock_wrlock: %s",strerror((errno)));
}
void RWLock::rdlock() const
{
    int err=pthread_rwlock_rdlock(&lock_rw);
    if(err!=0)
        throw CommonError("pthread_rwlock_rdlock: %s",strerror((errno)));
}
void RWLock::unlock() const
{
    int err=pthread_rwlock_unlock(&lock_rw);
    if(err!=0)
        throw CommonError("pthread_rwlock_unlock: %s",strerror((errno)));

}
