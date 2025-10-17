#include "uriReferals.h"

std::string dfsReferrer::_uriReferals::wdump()
{

    return "";
}
REF_getter<linkInfoDownReferrer> dfsReferrer::_uriReferals::getDownlinkOrNull(const SOCKET_id& id)
{
    auto i=m_downlinks.find(id);
    if(i!=m_downlinks.end()) return i->second;
    return NULL;
}
REF_getter<linkInfoDownReferrer> dfsReferrer::_uriReferals::getDownlinkOrCreate(const REF_getter<epoll_socket_info>& esi, const msockaddr_in& externalAddr,const std::set<msockaddr_in>& internalAddrs, const route_t& backRoute, bool* created)
{
    if(!esi.valid())
        throw CommonError("if(!esi.valid())");

    REF_getter<linkInfoDownReferrer> l(NULL);
    {
        auto i=m_downlinks.find(esi->id_);
        if(i!=m_downlinks.end())
        {
            l=i->second;
        }
    }
    if(l.valid())
    {

        if(!(l->backRoute_==backRoute) || l->externalAddr_mx_!=externalAddr || l->internalAddrs_mx!=internalAddrs)
        {
            *created=true;
        }
        else
        {
            *created=false;
        }
        l->backRoute_=backRoute;
        l->externalAddr_mx_=externalAddr;
        l->internalAddrs_mx=internalAddrs;
        return l;

    }
    else
    {
        l=new linkInfoDownReferrer(esi,externalAddr,internalAddrs,backRoute);
        {
            m_downlinks.insert(std::make_pair(esi->id_,l));
        }
        *created=true;
        return l;

    }
    return NULL;
}
