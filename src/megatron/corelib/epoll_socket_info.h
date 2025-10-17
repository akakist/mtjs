#pragma once

#include "networkMultiplexor.h"

#include <deque>
#include "REF.h"
#include "SOCKET_fd.h"
#include "webDumpable.h"
#include "SOCKET_id.h"
#include "route_t.h"
#include <optional>
#include <openssl/ssl.h>
#include "ISSL.h"
#include "msockaddr_in.h"

/**
* Wrapper for socket selector
*/
struct SECURE
{
    bool use_ssl=false;
    std::string cert_pn;
    std::string key_pn;
};
struct SECURE_CONTEXT: public Refcountable
{
    I_ssl *issl=nullptr;
    SSL_CTX * sslctx=nullptr;
    BIO *errBio=NULL;
};
struct P_msockaddr_in: public Refcountable
{
    msockaddr_in addr;
    P_msockaddr_in(const msockaddr_in& sa)
        : addr(sa)
    {

    }
};

class   socketBuffersOut
{
    RWLock lk;
    std::string container_;
    size_t cur_begin=0;
    /// if true socket must be closed after flush data

public:
    bool markedToDestroyOnSend_=false;
    void append(epoll_socket_info *esi, const std::string &s);
    void append_and_close(epoll_socket_info *esi, const std::string &s);
    size_t size();
    int send(const SOCKET_fd &fd, epoll_socket_info *esi);
    std::string getAll()
    {
        R_LOCK(lk);
        return container_;
    }
};
class epoll_socket_info;
/**
* Socket wrapper
*/
struct NetworkMultiplexor;

class epoll_socket_info:public Refcountable
#ifdef WEBDUMP
    , public WebDumpable
#endif
{

public:
    const int socketType_; /// SOCK_STREAM, SOCK_DGRAM
    enum STREAMTYPE
    {
        STREAMTYPE_ACCEPTED,
        STREAMTYPE_CONNECTED,
        STREAMTYPE_LISTENING,
        STREAMTYPE_UDP_ASSOC
    };

    /// socket type   _ACCEPTED,  _CONNECTED,  _LISTENING,
    const STREAMTYPE streamType_;

    /// unique ID
    SOCKET_id id_;

    REF_getter<SECURE_CONTEXT> secure_context;
    SSL *ssl;
    bool ssl_connected=false;


private:

public:
    /// filedescriptor
    SOCKET_fd fd_;

    bool closed();

    /// routing
    const route_t m_route;




    /// out buffer
    socketBuffersOut outBuffer_;

    /// in buffer
    struct _inBuffer
    {
        RWLock lk;
        std::string _mx_data;
        void append(const char* data, size_t size);
        size_t size();
        std::string extract_all();
    };
    _inBuffer inBuffer_;

    /// socket in ::connect state
    bool inConnection_;

    /// remote peer name
    std::optional<msockaddr_in> request_for_connect_;
    std::optional<msockaddr_in> local_name_;
    std::optional<msockaddr_in> remote_name_;

    std::optional<msockaddr_in> server_name_;
    /*
    */
    msockaddr_in &local_name()
    {
        if(local_name_.has_value())
            return *local_name_;
        msockaddr_in lname;
        socklen_t sl=lname.maxAddrLen();
        if(getsockname(CONTAINER(fd_), lname.addr(), &sl))
        {
            logErr2("getsockname: errno %d %s",errno,strerror(errno));
        }
        local_name_.emplace(lname);
        return *local_name_;

    }
    msockaddr_in& remote_name()
    {
        XTRY;
        msockaddr_in rname;
        socklen_t sr=rname.maxAddrLen();
        if(getpeername(CONTAINER(fd_), rname.addr(), &sr))
        {
            logErr2("getsockname: errno %d %s %s",errno,strerror(errno),_DMI().c_str());
        }
        remote_name_.emplace(rname);
        return *remote_name_;

        XPASS;
    }



    /// any text of socket for debugging
    const char* socketDescription_;

    ///  used to check buffer available for processing

    REF_getter<NetworkMultiplexor> multiplexor_;


    epoll_socket_info(const int& socketType, const STREAMTYPE &_streamtype,const SOCKET_id& _id,const SOCKET_fd& _fd, const route_t& _route,
                      const char* _socketDescription,
                      const REF_getter<NetworkMultiplexor>& _multiplexor, const REF_getter<SECURE_CONTEXT>& _secure_context);

    virtual ~epoll_socket_info();

    /// implementation of webDumpable (display in http)
    std::string wdump();
    std::string wname() {
        return "epoll_socket_info";
    }

    /// write buffer
    void write_buf(const void *s, const size_t &sz);
    void write_(const std::string&s);
    void write_and_close(const std::string&s);

    void close(const char *reason);

    std::map<int,REF_getter<Refcountable> > additions_;
    RWLock additions_lk;
#ifdef HAVE_KQUEUE
    bool ev_read_added_;
    bool ev_write_added_;
#elif defined(HAVE_EPOLL)
    bool ev_added;
#elif defined(HAVE_SELECT)

#else
#error "HAVE iundef22"
#endif
};


