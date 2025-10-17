#pragma once
#include "ioBuffer.h"
#include "SERVICE_id.h"
#include "REF.h"
#include "CONTAINER.h"
#include "SOCKET_id.h"
#include <set>

/// event routing

class ObjectHandlerPolled;
class ObjectHandlerThreaded;
class ListenerBase;

class LocalServiceRoute
{
public:
    SERVICE_id id;
};
class ListenerRoute
{
public:
    ListenerBase* id;
};
class ThreadRoute
{
public:
    int id;
};

class SocketRoute
{
public:
    SOCKET_id id;
};
class RemoteAddrRoute
{
public:
    SOCKET_id addr;
};

class ObjectHandlerRoutePolled
{
public:
    ObjectHandlerPolled*  addr_;
};
class ObjectHandlerRouteThreaded
{
public:
    ObjectHandlerThreaded*  addr_;
};
class Route
{
public:
    enum RouteType
    {
        LOCALSERVICE            =1,
        REMOTEADDR              =2,
        OBJECTHANDLER_POLLED    =3,
        SOCKETROUTE             =4,
        OBJECTHANDLER_THREADED  =5,
        THREAD                  =6,
        LISTENER                =7
    };
    RouteType type;
    Route(RouteType t):type(t) {}

    union
    {
        LocalServiceRoute localServiceRoute;
        ListenerRoute listenerRoute;
        ThreadRoute threadRoute;
        SocketRoute socketRoute;
        RemoteAddrRoute remoteAddrRoute;
        ObjectHandlerRoutePolled objectHandlerRoutePolled;
        ObjectHandlerRouteThreaded objectHandlerRouteThreaded;

    };


    void pack(outBuffer&o) const ;
    void unpack(inBuffer&o);
    std::string dump()const;
};

class route_t
{

    std::deque<Route> m_container;
public:
    route_t() {}
    std::string dump() const;
    route_t(const SERVICE_id& id);
    route_t(ObjectHandlerPolled* id);
    route_t(ObjectHandlerThreaded* id);
    route_t(ListenerBase* id);
    route_t(const std::string &javaCookie,ObjectHandlerPolled* id);
    route_t(const std::string &javaCookie,ObjectHandlerThreaded* id);

    void push_front(const Route& v);
    void push_front(const route_t& r);
    Route pop_front();
    size_t size()const ;
    void pack(outBuffer&o) const;
    void unpack(inBuffer&o);
    int operator<(const route_t& a) const;
    bool operator==(const route_t& a) const;
    std::string getLastJavaCookie() const;

};
outBuffer& operator << (outBuffer& b, const route_t & r);
inBuffer& operator >> (inBuffer&b,route_t & r);
