#include "IUtils.h"
#include <unistd.h>
#ifdef _WIN32
#include <Winsock2.h>
#endif
#include "epoll_socket_info.h"
#ifdef __linux__
#include <sys/epoll.h>
#endif
#ifdef __APPLE__

#endif
#ifdef _WIN32
#include "compat_win32.h"
#include <winsock2.h>
#endif
#include "mutexInspector.h"


std::string epoll_socket_info::wdump()
{
    return "";
}
void epoll_socket_info::write_buf(const void *s, const size_t &sz)
{
    if(!closed())
    {
        // printf("Write: %s\n",s.c_str());
        XTRY;
        {
            XTRY;
            outBuffer_.append(this,std::string((char*)s,sz));
            XPASS;
        }
        XPASS;
    }

}
void epoll_socket_info::write_(const std::string&s)
{
    MUTEX_INSPECTOR;
    XTRY;
    // logErr2("write: %s",s.c_str());
    if(!closed())
    {
        // printf("Write: %s\n",s.c_str());
        XTRY;
        {
            XTRY;
            outBuffer_.append(this,s);
            XPASS;
        }
        XPASS;
    }

    XPASS;
}
void epoll_socket_info::write_and_close(const std::string&s)
{
    MUTEX_INSPECTOR;
    XTRY;
    if(!closed())
    {

        XTRY;
        {
            XTRY;
            outBuffer_.append_and_close(this,s);
            XPASS;
        }
        XPASS;
    }

    XPASS;
}

void epoll_socket_info::close(const char* reason)
{
    MUTEX_INSPECTOR;

    DBG(logErr2("epoll_socket_info::close %s",reason));
    XTRY;
    std::string sType;
    if(CONTAINER(fd_)==-1)
    {
        return;
    }

    if(this->streamType_==epoll_socket_info::STREAMTYPE_ACCEPTED)
    {
        sType="_ACCEPTED";
    }
    else if(this->streamType_==epoll_socket_info::STREAMTYPE_CONNECTED)
    {
        sType="_CONNECTED";
    }
    else if(this->streamType_==epoll_socket_info::STREAMTYPE_LISTENING)
    {
        sType="_LISTENING";
    }
    DBG(logErr2("close reason: %s remote name %s type %s fd %d",reason,remote_name().dump().c_str(),sType.c_str(),CONTAINER(fd_)));
    if(closed())
    {
        DBG(logErr2("socket already closed %s",_DMI().c_str()));
        return;
    }
    XPASS;

    if(CONTAINER(fd_)!=-1)
    {
#ifdef _WIN32
        if(!::closesocket(CONTAINER(m_fd)))
#else
        if(!::close(CONTAINER(fd_)))
#endif
        {
        }
        // CONTAINER(id_)=-1;
        CONTAINER(fd_)=-1;

    }

}

epoll_socket_info::~epoll_socket_info()
{
    // DBG(logErr2("~epoll_socket_info()"));

    try {

        if(secure_context.valid())
        {
            if(ssl!=nullptr)
            {
                secure_context->issl->SSL_free(ssl);
                ssl=nullptr;
            }
        }
        if(CONTAINER(fd_)!=-1)
        {
            DBG(logErr2(" %s on nonclosed sock %s",__PRETTY_FUNCTION__,socketDescription_));
#ifdef _WIN32
            if(!::closesocket(CONTAINER(m_fd)))
#else
            if(!::close(CONTAINER(fd_)))
#endif
            {
//                printf(RED("::close() error %d %s"),errno,strerror(errno));
            }
            // CONTAINER(id_)=-1;
            CONTAINER(fd_)=-1;
        }


    } catch(...) {}
}






void socketBuffersOut::append(epoll_socket_info* esi,const std::string&s)
{
    MUTEX_INSPECTOR;
    {
        W_LOCK(lk);
        container_+=s;
    }
    esi->multiplexor_->sockStartWrite(esi);

}
void socketBuffersOut::append_and_close(epoll_socket_info* esi,const std::string&s)
{
    MUTEX_INSPECTOR;
    {
        W_LOCK(lk);
        container_+=s;
        // logErr2("append_and_close sz %d",container_.size());
        markedToDestroyOnSend_=true;
    }
    esi->multiplexor_->sockStartWrite(esi);

}

size_t socketBuffersOut::size()
{
    MUTEX_INSPECTOR;
    R_LOCK(lk);
    return container_.size();
}
int socketBuffersOut::send(const SOCKET_fd &fd, epoll_socket_info* esi)
{
    MUTEX_INSPECTOR;
    int res=0;
    int restSize=0;
    {
        W_LOCK(lk);
        if(container_.size())
        {
            if(esi->secure_context.valid())
            {
                res=esi->secure_context->issl->SSL_write(esi->ssl,container_.data(),container_.size());
            }
            else
            {
                res=::send(CONTAINER(fd),container_.data(),container_.size(),0);
                // DBG(printf("::send res %s\n",container_.c_str()));
            }

            if(res>0)
            {
                if(res==container_.size())
                {
                    // DBG(logErr2("container_.clear();"));
                    container_.clear();
                }
                else
                {
                    DBG(logErr2("container_=container_.substr(res,container_.size()-res);"));
                    container_=container_.substr(res,container_.size()-res);
                    DBG(logErr2("container remains %d",container_.size()));
                }
            }
        }

        if(container_.size()==0)
            esi->multiplexor_->sockStopWrite(esi);

    }
    return res;
}

bool epoll_socket_info::closed()
{
    return CONTAINER(fd_)==-1;
}

epoll_socket_info::epoll_socket_info(const int &_socketType, const STREAMTYPE &_streamtype, const SOCKET_id& _id, const SOCKET_fd& _fd,
                                     const route_t& _route, const char *_socketDescription, const REF_getter<NetworkMultiplexor> &_multiplexor,const REF_getter<SECURE_CONTEXT>& _secure_context):
    socketType_(_socketType),
    streamType_(_streamtype),
    id_(_id),
    fd_(_fd),
    m_route(_route),
    inConnection_(false),
    socketDescription_(_socketDescription),
    multiplexor_(_multiplexor),
    secure_context(_secure_context)
#ifdef HAVE_KQUEUE
    ,
    ev_read_added_(false),
    ev_write_added_(false)
#elif defined(HAVE_EPOLL)
    ,
    ev_added(false)
#elif defined(HAVE_SELECT)
#else
#error "HAVE UNDEF"
#endif

{
    // DBG(logErr2("epoll_socket_info()"));
    if(secure_context.valid())
    {
        ssl=secure_context->issl->SSL_new(secure_context->sslctx);
        if(ssl==nullptr)
            throw CommonError("SSL_new failed");
        auto r=secure_context->issl->SSL_set_fd(ssl,CONTAINER(fd_));
        if(r==0)
            throw CommonError("SSL_set_fd failed");
    }
}

void epoll_socket_info::_inBuffer::append(const char* data, size_t size)
{
    W_LOCK(lk);
    _mx_data.append(data,size);
}
size_t epoll_socket_info::_inBuffer::size()
{
    MUTEX_INSPECTOR;
    R_LOCK(lk);
    return _mx_data.size();
}
std::string epoll_socket_info::_inBuffer::extract_all()
{
    MUTEX_INSPECTOR;
    W_LOCK(lk);
//    return std::move(_mx_data);
//    std::string ret=std::move(_mx_data);
    std::string ret=_mx_data;
    _mx_data.clear();
    return ret;

}
