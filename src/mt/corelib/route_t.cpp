#include "IUtils.h"
#include "objectHandler.h"
#include "mutexInspector.h"
#include "listenerBase.h"
#include "commonError.h"
#include "route_t.h"
std::string Route::dump()const
{
    std::string r;
    switch (type)
    {
    case LOCALSERVICE:
        return (std::string)"LOCAL("+iUtils->genum_name(localServiceRoute.id)+")";
        break;
    case REMOTEADDR:
        return "REMOTE";
        break;
    case OBJECTHANDLER_POLLED:
        return "OBJECTHANDLER_POLLED ("+objectHandlerRouteThreaded.addr_->name+")";
        break;
    case OBJECTHANDLER_THREADED:
        return "OBJECTHANDLER_THREADED ("+objectHandlerRoutePolled.addr_->name+")";
        break;
    case SOCKETROUTE:
        return "SOCKETROUTE";
        break;
    case THREAD:
        return "THREAD";
        break;
    case LISTENER:
        return "LISTENER ("+listenerRoute.id->listenerName_+")";
        break;

    default:
        throw CommonError("invalid route");

    }


}

std::string route_t::dump() const
{
    std::vector<std::string> ret;
    ret.reserve(m_container.size());
    for(size_t i=0; i<m_container.size(); i++)
    {
        ret.push_back(m_container[i].dump());
    }

    return iUtils->join("|",ret);

}


void Route::pack(outBuffer&o) const
{
    o << (int)type;
    switch (type)
    {
    case LOCALSERVICE:
        o<<(uint64_t)localServiceRoute.id;
        break;
    case REMOTEADDR:
        o<<remoteAddrRoute.addr;
        break;
    case OBJECTHANDLER_POLLED:
        o<<(uint64_t)objectHandlerRoutePolled.addr_;
        break;
    case SOCKETROUTE:
        o<<socketRoute.id;
        break;
    case OBJECTHANDLER_THREADED:
        o<<(uint64_t)objectHandlerRouteThreaded.addr_;
        break;
    case THREAD:
        o<<threadRoute.id;
        break;
    case LISTENER:
        o<<(uint64_t)listenerRoute.id;
        break;

    default:
        throw CommonError("invalid route");

    }

}
void Route::unpack(inBuffer&o)
{
    int t;
    o>>t;
    type=(RouteType)t;
    switch (type)
    {
    case LOCALSERVICE:
    {
        uint64_t n;
        o>>n;
        localServiceRoute.id=n;

    }
    break;
    case REMOTEADDR:
        o>>remoteAddrRoute.addr;
        break;
    case OBJECTHANDLER_POLLED:
    {
        uint64_t n;
        o>>n;
        objectHandlerRoutePolled.addr_=(ObjectHandlerPolled*) n;

    }
    break;
    case OBJECTHANDLER_THREADED:
    {
        uint64_t n;
        o>>n;
        objectHandlerRouteThreaded.addr_=(ObjectHandlerThreaded*) n;

    }
    break;
    case SOCKETROUTE:
        o>>socketRoute.id;
        break;
    case THREAD:
        o>>threadRoute.id;
        break;
    case LISTENER:
    {
        uint64_t n;
        o>>n;
        listenerRoute.id=(ListenerBase*) n;

    }
    break;

    default:
        throw CommonError("invalid route");

    }
}




inBuffer& operator >> (inBuffer&b,route_t & r)
{

    r.unpack(b);
    return b;
}
outBuffer& operator << (outBuffer& b, const route_t & r)
{

    r.pack(b);
    return b;
}


route_t::route_t(const SERVICE_id& id)
{

    Route r(Route::LOCALSERVICE);
    r.localServiceRoute.id=id;
    m_container.push_back(r);
}
route_t::route_t(ListenerBase* id)
{
    Route r(Route::LISTENER);
    r.listenerRoute.id=id;
    m_container.push_back(r);
}
route_t::route_t(ObjectHandlerPolled* id)
{
    Route r(Route::OBJECTHANDLER_POLLED);
    r.objectHandlerRoutePolled.addr_=id;
    m_container.push_back(r);
}
route_t::route_t(ObjectHandlerThreaded* id)
{
    Route r(Route::OBJECTHANDLER_THREADED);
    r.objectHandlerRouteThreaded.addr_=id;
    m_container.push_back(r);
}
route_t::route_t(const std::string &javaCookie,ObjectHandlerPolled* id)
{
    {
        Route r(Route::LOCALSERVICE);
        r.localServiceRoute.id=atoi(javaCookie.c_str());
        m_container.push_back(r);
    }
    {
        Route r(Route::OBJECTHANDLER_POLLED);
        r.objectHandlerRoutePolled.addr_=id;
        m_container.push_back(r);
    }

}
route_t::route_t(const std::string &javaCookie,ObjectHandlerThreaded* id)
{
    {
        Route r(Route::LOCALSERVICE);
        r.localServiceRoute.id=atoi(javaCookie.c_str());
        m_container.push_back(r);
    }
    {
        Route r(Route::OBJECTHANDLER_THREADED);
        r.objectHandlerRouteThreaded.addr_=id;
        m_container.push_back(r);
    }
}



size_t route_t::size()const
{
    return m_container.size();
}
void route_t::pack(outBuffer&o) const
{

    XTRY;
    o<<m_container.size();
    for(size_t i=0; i<m_container.size(); i++)
    {
        m_container[i].pack(o);
    }
    XPASS;
}
void route_t::unpack(inBuffer&o)
{

    XTRY;
    size_t sz;
    {
        XTRY;
        o>>sz;
        XPASS;

    }
    // m_container.reserve(sz);
    for(size_t i=0; i<sz; i++)
    {
        XTRY;
        Route::RouteType type;
        int t;
        {
            XTRY;
            o>>t;
            type=(Route::RouteType)t;
            XPASS;
        }

        switch(type)
        {
        case Route::LISTENER:
            throw CommonError("route unpack for listener");
            break;
        case Route::LOCALSERVICE:
        {
            XTRY;
            Route r(Route::LOCALSERVICE);
            o>>r.localServiceRoute.id;
            m_container.push_back(r);
            XPASS;
        }
        break;
        case Route::THREAD:
        {
            XTRY;
            Route r(Route::THREAD);
            o>>r.threadRoute.id;
            m_container.push_back(r);
            XPASS;
        }
        break;
        case Route::REMOTEADDR:
        {
            XTRY;
            Route r(Route::REMOTEADDR);
            o>>r.remoteAddrRoute.addr;
            m_container.push_back(r);
            XPASS;
        }
        break;
        case Route::OBJECTHANDLER_POLLED:
        {
            XTRY;
            Route r(Route::OBJECTHANDLER_POLLED);
            uint64_t z;
            o>>z;
            r.objectHandlerRoutePolled.addr_=(ObjectHandlerPolled *)z;
            m_container.push_back(r);
            XPASS;
        }
        break;
        case Route::OBJECTHANDLER_THREADED:
        {
            XTRY;
            Route r(Route::OBJECTHANDLER_THREADED);
            uint64_t z;
            o>>z;
            r.objectHandlerRouteThreaded.addr_=(ObjectHandlerThreaded *)z;
            m_container.push_back(r);
            XPASS;
        }
        break;
        case Route::SOCKETROUTE:
        {
            XTRY;
            Route r(Route::SOCKETROUTE);
            o>>r.socketRoute.id;
            m_container.push_back(r);
            XPASS;
        }
        break;
        }
        XPASS;
    }
    {
        XTRY;
        XPASS;
    }
    XPASS;
}
int route_t::operator<(const route_t& a) const
{

    XTRY;
    if(a.m_container.size()!=m_container.size()) return a.m_container.size()<m_container.size();
    for(size_t i=0; i<m_container.size(); i++)
    {
        if(a.m_container[i].type!=m_container[i].type) return a.m_container[i].type < m_container[i].type;
        switch(a.m_container[i].type)
        {
        case Route::LOCALSERVICE:
        {
            if(a.m_container[i].localServiceRoute.id != m_container[i].localServiceRoute.id)
                return a.m_container[i].localServiceRoute.id < m_container[i].localServiceRoute.id;
        }
        break;
        case Route::LISTENER:
        {
            if(a.m_container[i].listenerRoute.id != m_container[i].listenerRoute.id)
                return a.m_container[i].listenerRoute.id < m_container[i].listenerRoute.id;
        }
        break;

        case Route::THREAD:
        {
            if(a.m_container[i].threadRoute.id != m_container[i].threadRoute.id)
                return a.m_container[i].threadRoute.id < m_container[i].threadRoute.id;
        }
        break;
        case Route::REMOTEADDR:
        {
            if(a.m_container[i].remoteAddrRoute.addr != m_container[i].remoteAddrRoute.addr)
                return a.m_container[i].remoteAddrRoute.addr < m_container[i].remoteAddrRoute.addr;
        }
        break;
        case Route::OBJECTHANDLER_POLLED:
        {
            if(a.m_container[i].objectHandlerRoutePolled.addr_ != m_container[i].objectHandlerRoutePolled.addr_)
                return a.m_container[i].objectHandlerRoutePolled.addr_ < m_container[i].objectHandlerRoutePolled.addr_;
        }
        break;
        case Route::OBJECTHANDLER_THREADED:
        {
            if(a.m_container[i].objectHandlerRouteThreaded.addr_ != m_container[i].objectHandlerRouteThreaded.addr_)
                return a.m_container[i].objectHandlerRouteThreaded.addr_ < m_container[i].objectHandlerRouteThreaded.addr_;
        }
        break;
        case Route::SOCKETROUTE:
        {
            if(a.m_container[i].socketRoute.id != m_container[i].socketRoute.id)
                return a.m_container[i].socketRoute.id < m_container[i].socketRoute.id;
        }
        break;
        default:
            throw CommonError("invalid route type %d",a.m_container[i].type);
        }
    }
    XPASS;
    return 0;
}


bool route_t::operator==(const route_t& a) const
{

    XTRY;
    if(a.m_container.size()!=m_container.size()) return false;
    for(size_t i=0; i<m_container.size(); i++)
    {
        auto at=a.m_container[i].type;
        if(at!=m_container[i].type) return false;
        switch(at)
        {
        case Route::LISTENER:
        {
            if(a.m_container[i].listenerRoute.id != m_container[i].listenerRoute.id)
                return false;
        }
        break;
        case Route::LOCALSERVICE:
        {
            if(a.m_container[i].localServiceRoute.id != m_container[i].localServiceRoute.id)
                return false;
        }
        break;
        case Route::THREAD:
        {
            if(a.m_container[i].threadRoute.id != m_container[i].threadRoute.id)
                return false;
        }
        break;
        case Route::REMOTEADDR:
        {
            if(CONTAINER(a.m_container[i].remoteAddrRoute.addr) != CONTAINER(m_container[i].remoteAddrRoute.addr))
                return false;
        }
        break;
        case Route::OBJECTHANDLER_POLLED:
        {
            if(a.m_container[i].objectHandlerRoutePolled.addr_ != m_container[i].objectHandlerRoutePolled.addr_)
                return false;
        }
        break;
        case Route::OBJECTHANDLER_THREADED:
        {
            if(a.m_container[i].objectHandlerRouteThreaded.addr_ != m_container[i].objectHandlerRouteThreaded.addr_)
                return false;
        }
        break;
        case Route::SOCKETROUTE:
        {
            if(CONTAINER(a.m_container[i].socketRoute.id) != CONTAINER(m_container[i].socketRoute.id))
                return false;
        }
        break;
        default:
            throw CommonError("invalid route type %d",at);
        }
    }
    XPASS;
    return true;
}
std::string route_t::getLastJavaCookie() const
{
    if(m_container.size()!=1)
        throw CommonError("route_t: if(m_container.size()!=1)");
    auto r=m_container[0];
    {
        if(r.type==Route::LOCALSERVICE)
        {
            return std::to_string(r.localServiceRoute.id);
        }

    }
    throw CommonError("invalid case");

}
