#pragma once
#include "IUtils.h"
#include "ioBuffer.h"
#include "mutexInspector.h"
#include "route_t.h"
#include "IInstance.h"
#include "IRPC.h"
#include <stdarg.h>

#include "msockaddr_in.h"
#include "Events/System/Net/rpcEvent.h"

/// some tools used in DFS

inline unsigned short getRpcExternalListenPortMain(IInstance* iInstance)
{

    auto unk=iInstance->getServiceOrCreate(ServiceEnum::RPC);
    if(!unk) throw CommonError("!if(!unk)"+_DMI());
    IRPC* rpc=dynamic_cast<IRPC*>(unk);
    if(!rpc) throw CommonError("!rpc"+_DMI());
    unsigned short port=rpc->getExternalListenPortMain();
#ifndef __MOBILE__
    if(port==0)
        throw CommonError("port==0: rpc->getExternalListenPortMain() return 0"+_DMI());
#endif

    return port;
}

inline std::set<msockaddr_in> getRpcInternalListenAddrs(IInstance* iInstance)
{
    auto unk=iInstance->getServiceOrCreate(ServiceEnum::RPC);
    if(!unk) throw CommonError("!if(!unk)"+_DMI());
    IRPC* irpc=dynamic_cast<IRPC*>(unk);
    if(!irpc) throw CommonError("if(!irpc) "+_DMI());

    return irpc->getInternalListenAddrs();
}
inline std::string dump(const std::set<unsigned short> listenPort)
{
    std::vector<std::string> v;
    v.reserve(listenPort.size());
    for(auto& i:listenPort)
    {
        v.push_back(std::to_string(ntohs(i)));
    }
    return iUtils->join(",",v);
}
inline std::string dump(const std::set<msockaddr_in> listenPort)
{
    std::vector<std::string> v;
    v.reserve(listenPort.size());
    for(auto& i:listenPort)
    {
        v.push_back(i.dump());
    }
    return iUtils->join(",",v);
}


inline std::vector<std::string> conv_s2v(const std::set<std::string>&v)
{
    std::vector<std::string> out;
    out.reserve(v.size());
    for(auto& i:v)
    {
        out.push_back(i);
    }
    return out;
}
inline bool UuidIsCyclic(const std::vector<msockaddr_in>&uuid)
{
    std::map<msockaddr_in,int> m;
    for(size_t i=0; i<uuid.size(); i++)
    {
        if(!m.count(uuid[i]))
            m[uuid[i]]=1;
        else
            return true;
    }
    return false;
}

inline route_t poppedFrontRoute(const route_t&r)
{
    route_t rt=r;
    rt.pop_back();
    return rt;
}
template <class T>
T mabs(T a)
{
    if(a<0) return -a;
    return a;
}

template <class T>
REF_getter<refbuffer> outV(T a)
{
    outBuffer o;
    o<<a;
    return o.asString();

}
template <class T1,class T2>
REF_getter<refbuffer> outV(const T1& a, const T2& b)
{
    outBuffer o;
    o<<a<<b;
    return o.asString();

}
template <class T1,class T2,class T3>
REF_getter<refbuffer> outV(const T1& a, const T2& b, const T3& c)
{
    outBuffer o;
    o<<a<<b<<c;
    return o.asString();

}
template <class T1,class T2,class T3,class T4>
REF_getter<refbuffer> outV(const T1& a, const T2& b, const T3& c,const T4& d)
{
    outBuffer o;
    o<<a<<b<<c<<d;
    return o.asString();

}
template <class T1,class T2,class T3,class T4,class T5>
REF_getter<refbuffer> outV(const T1& a, const T2& b, const T3& c,const T4& d,const T5& e)
{
    outBuffer o;
    o<<a<<b<<c<<d<<e;
    return o.asString();
}
template <class T1,class T2,class T3,class T4,class T5,class T6>
REF_getter<refbuffer> outV(const T1& a, const T2& b, const T3& c,const T4& d,const T5& e,const T6& f)
{
    outBuffer o;
    o<<a<<b<<c<<d<<e<<f;
    return o.asString();
}


inline std::string sqlDbName(const std::string& nm)
{
    std::string n;
    for(size_t i=0; i<nm.size(); i++)
    {
        if(isalnum(nm[i]))
            n+=nm[i];
    }
    return iUtils->gCacheDir()+"/"+n+".db";
}
