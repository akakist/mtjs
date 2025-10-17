#pragma once

#include "e_poll.h"
#include <vector>
#ifdef __FreeBSD__
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#endif
#ifdef __linux__
#include <sys/epoll.h>
#endif

#ifdef __MACH__
#include <sys/select.h>
#include <sys/event.h>
#endif
#include "REF.h"
#include "commonError.h"
class epoll_socket_info;

struct NetworkMultiplexor: public Refcountable
{
    int m_handle;
private:

public:
    NetworkMultiplexor()
    {
#ifdef  HAVE_KQUEUE
        m_handle=kqueue();
        if(m_handle==-1)
            throw CommonError("kqueue(): errno %d",errno);
#elif defined(HAVE_EPOLL)
        m_handle=epoll_create(100);
        if(m_handle==-1)
            throw CommonError("epoll_create(): errno %d",errno);
#elif defined (HAVE_SELECT)
#else
#error "asdfasfdasfasdasd"
#endif
    }

public:

    ~NetworkMultiplexor();

    void sockAddReadOnNew(epoll_socket_info *esi);
    void sockStartWrite(epoll_socket_info* esi);
    void sockStopWrite(epoll_socket_info* esi);
    void sockAddRWOnNew(epoll_socket_info* esi);


};


