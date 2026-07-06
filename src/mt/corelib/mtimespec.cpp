#include "mtimespec.h"
#include "commonError.h"
#include <errno.h>
#ifdef _WIN32
#include <sys/timeb.h>
#endif
#include <time.h>
mtimespec::mtimespec(long sec,long nsec)
{
    tv_sec=sec;
    tv_nsec=nsec;
}
mtimespec::mtimespec()
{
    tv_sec=0;
    tv_nsec=0;
}
void nanotime(timespec* t)
{
    /// GetCurrentTime
#if !defined(WIN32)
    if(clock_gettime(CLOCK_REALTIME,t))
    {
        throw CommonError("clock_gettime: errno %d",errno);
    }
#else
    struct timeb tb;
    ftime(&tb);
    t->tv_sec=tb.time;
    t->tv_nsec=tb.millitm*1000000;
#endif
}
int operator<(const mtimespec& a,const mtimespec& b)
{
    if(a.tv_nsec>_nano_) throw CommonError("if(a.tv_nsec>nano) ");
    if(b.tv_nsec>_nano_) throw CommonError("if(b.tv_nsec>nano) ");

    if(a.tv_sec!=b.tv_sec)
        return a.tv_sec<b.tv_sec;
    if(a.tv_nsec!=b.tv_nsec)
        return a.tv_nsec<b.tv_nsec;
    return 0;

    return false;
}
int operator<=(const mtimespec& a,const mtimespec& b)
{
    if(a.tv_nsec>_nano_) throw CommonError("if(a.tv_nsec>nano) ");
    if(b.tv_nsec>_nano_) throw CommonError("if(b.tv_nsec>nano) ");

    if(a.tv_sec<=b.tv_sec) return true;
    else if(a.tv_sec==b.tv_sec)
    {
        if(a.tv_nsec<=b.tv_nsec) return true;
    }
    return false;
}

mtimespec operator+(const mtimespec& a,const mtimespec& b)
{
    mtimespec r;
    r.tv_sec=a.tv_sec+b.tv_sec+ ((a.tv_nsec+b.tv_nsec)/_nano_);
    r.tv_nsec=(a.tv_nsec+b.tv_nsec)%_nano_;
    return r;
}
mtimespec operator-(const mtimespec& a,const mtimespec& b)
{
    mtimespec r;
    r.tv_sec=a.tv_sec-b.tv_sec;
    r.tv_nsec=a.tv_nsec-b.tv_nsec;
    if(r.tv_nsec<0)
    {
        r.tv_nsec+=_nano_;
        r.tv_sec--;
    }
    return r;
}
