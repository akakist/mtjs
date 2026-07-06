#include "networkMultiplexor.h"
#include <unistd.h>
#ifdef __linux__
#include <sys/epoll.h>
#endif
#include "epoll_socket_info.h"

NetworkMultiplexor::~NetworkMultiplexor()
{
#ifdef HAVE_EPOLL
//    logErr2("close m_handle %d",m_handle);
    if(::close(m_handle))
        logErr2("close: %d", errno);
#endif
#ifdef HAVE_KQUEUE
    if(::close(m_handle))
        logErr2("close: %d %s", errno,strerror(errno));
#endif
}



void NetworkMultiplexor::sockStartWrite(epoll_socket_info* esi)
{
    XTRY;
#ifdef HAVE_EPOLL
    struct epoll_event evz;
    evz.data.u64 = CONTAINER(esi->id_);
    evz.events = EPOLLOUT|EPOLLIN;
    // int fd=CONTAINER(esi->fd_);
    bool added=esi->ev_added;
    int flag=added?EPOLL_CTL_MOD:EPOLL_CTL_ADD;
    esi->ev_added=true;
    if(CONTAINER(esi->fd_)!=-1)
    {
        if (epoll_ctl(m_handle, flag, CONTAINER(esi->fd_), &evz) < 0)
        {
            // logErr2("epoll_ctl add: socket '%d' - errno %d %s %s",CONTAINER(esi->fd_), errno,__PRETTY_FUNCTION__, strerror(errno));
            if(!esi->closed())
            {
                esi->close("EPOLL_CTL_ADD");
            }
        }
    }
#elif defined (HAVE_KQUEUE)

    int fd=CONTAINER(esi->fd_);
    if(fd!=-1)
    {
        int flags=EV_ENABLE;
        if(!esi->ev_write_added_)
        {
            flags|=EV_ADD;
            esi->ev_write_added_=true;
        }


        struct kevent ev;
        EV_SET(&ev,fd,EVFILT_WRITE,flags,0,0,(void*)(long)CONTAINER(esi->id_));
        timespec ts;
        ts.tv_nsec=0;
        ts.tv_sec=0;
        kevent(m_handle,&ev,1,NULL,0,&ts);

    }
    else throw CommonError("fd==-1");
#elif defined(HAVE_SELECT)
#else
#error "asdafsdfads"
#endif

    XPASS;

}
void NetworkMultiplexor::sockStopWrite(epoll_socket_info* esi)
{
    XTRY;
#ifdef HAVE_EPOLL
    struct epoll_event evz;
    evz.data.u64 = CONTAINER(esi->id_);
    evz.events = EPOLLIN;
    int fd=CONTAINER(esi->fd_);
    bool added=esi->ev_added;
    int flag=added?EPOLL_CTL_MOD:EPOLL_CTL_ADD;
    esi->ev_added=true;
    if(fd!=-1)
    {
        if (epoll_ctl(m_handle, flag, fd, &evz) < 0)
        {
            // logErr2("epoll_ctl add: socket '%d' - errno %d %s %s",CONTAINER(esi->fd_), errno,__PRETTY_FUNCTION__,strerror(errno));
            if(!esi->closed())
            {
                esi->close("EPOLL_CTL_ADD");
            }
        }
    }
#elif defined (HAVE_KQUEUE)

//    return;
    int fd=CONTAINER(esi->fd_);
    if(fd!=-1)
    {
        {
            struct kevent ev;
            EV_SET(&ev,fd,EVFILT_WRITE,EV_DISABLE,0,0,(void*)(long)CONTAINER(esi->id_));
            timespec ts;
            ts.tv_nsec=0;
            ts.tv_sec=0;
            kevent(m_handle,&ev,1,NULL,0,&ts);
        }
    }
    else throw CommonError("fd==-1");

#elif defined(HAVE_SELECT)
#else
#error "wretwertwe"
#endif

    XPASS;

}
void NetworkMultiplexor::sockAddRWOnNew(epoll_socket_info* esi)
{
//    logErr2("added sock to epoll %d %s",esi->m_fd,__PRETTY_FUNCTION__);
#ifdef HAVE_EPOLL
    {
        struct epoll_event evtz {};
        evtz.events=EPOLLIN|EPOLLOUT;
        evtz.data.u64= static_cast<uint64_t>(CONTAINER(esi->id_));
        bool added=esi->ev_added;
        int flag=added?EPOLL_CTL_MOD:EPOLL_CTL_ADD;
        esi->ev_added=true;

        if (epoll_ctl(m_handle, flag, CONTAINER(esi->fd_), &evtz) < 0)
        {
            logErr2("epoll_ctl mod: socket '%d' - errno %d %s",CONTAINER(esi->fd_), errno,strerror(errno));
        }

    }

#elif defined(HAVE_KQUEUE)
    {
        int flags=EV_ENABLE;
        if(!esi->ev_write_added_)
        {
            flags|=EV_ADD;
            esi->ev_write_added_=true;
        }

        struct kevent ev;
        EV_SET(&ev,CONTAINER(esi->fd_),EVFILT_WRITE,flags,0,0,(void*)(long)CONTAINER(esi->id_));
        timespec ts;
        ts.tv_nsec=0;
        ts.tv_sec=0;
        kevent(m_handle,&ev,1,NULL,0,&ts);
    }
    {
        int flags=EV_ENABLE;
        if(!esi->ev_read_added_)
        {
            flags|=EV_ADD;
            esi->ev_read_added_=true;
        }

        struct kevent ev;
        EV_SET(&ev,CONTAINER(esi->fd_),EVFILT_READ,flags,0,0,(void*)(long)CONTAINER(esi->id_));
        timespec ts;
        ts.tv_nsec=0;
        ts.tv_sec=0;
        kevent(m_handle,&ev,1,NULL,0,&ts);
    }
#elif defined(HAVE_SELECT)
#else
#error "asdasdasdfafsdafdswqreqwr"
#endif

}
void NetworkMultiplexor::sockAddReadOnNew(epoll_socket_info* esi)
{

//    logErr2("added sock to epoll %d %s",esi->m_fd,__PRETTY_FUNCTION__);

    XTRY;
#ifdef HAVE_EPOLL
    struct epoll_event evz;
    evz.data.u64 = CONTAINER(esi->id_);
    evz.events = EPOLLIN;
    int fd=CONTAINER(esi->fd_);
    bool added=esi->ev_added;
    int flag=added?EPOLL_CTL_MOD:EPOLL_CTL_ADD;
    esi->ev_added=true;
    if(fd!=-1)
    {
        if (epoll_ctl(m_handle, flag, fd, &evz) < 0)
        {
            // logErr2("epoll_ctl add: socket '%d' - errno %d %s %s %s",CONTAINER(esi->fd_), errno,__PRETTY_FUNCTION__,strerror(errno));
            if(!esi->closed())
            {
                esi->close("EPOLL_CTL_ADD");
            }
        }
    }
#elif defined (HAVE_KQUEUE)

    int fd=CONTAINER(esi->fd_);
    if(fd!=-1)
    {
        {
            int flags=EV_ENABLE;
            if(!esi->ev_read_added_)
            {
                flags|=EV_ADD;
                esi->ev_read_added_=true;
            }

            struct kevent ev;
            EV_SET(&ev,fd,EVFILT_READ,flags,0,0,(void*)(long)CONTAINER(esi->id_));
            timespec ts;
            ts.tv_nsec=0;
            ts.tv_sec=0;
            kevent(m_handle,&ev,1,NULL,0,&ts);
        }
    }
    else throw CommonError("fd==-1");
#elif defined(HAVE_SELECT)
#else
#error "rr4rrwe34rp"
#endif

    XPASS;
}
