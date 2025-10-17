#pragma once
#include <string>
#include "epoll_socket_info.h"

#include "webDumpable.h"
#include "IUtils.h"
struct linkInfoDownReferrer: public Refcountable
#ifdef WEBDUMP
    ,public WebDumpable
#endif
{
    REF_getter<epoll_socket_info> esi_;
    msockaddr_in externalAddr_mx_;
    std::set<msockaddr_in> internalAddrs_mx;
    route_t backRoute_;
    time_t lastReplaceByDownlink_;

    linkInfoDownReferrer(const REF_getter<epoll_socket_info> &_esi,const msockaddr_in& _externalAddr, const std::set<msockaddr_in> &_internalAddrs, const route_t &_backRoute)
        :esi_(_esi),externalAddr_mx_(_externalAddr), internalAddrs_mx(_internalAddrs),backRoute_(_backRoute), lastReplaceByDownlink_(time(NULL))
    {}
    std::string wdump()
    {
        return "";
    }
    std::string wname() {
        return "linkInfoDown";
    }


};
