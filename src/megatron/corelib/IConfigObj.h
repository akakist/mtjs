#pragma once
#include <set>
#include <string>
#include "msockaddr_in.h"
#include "Real.h"

/// Interface for megatron config


class IConfigObj
{
public:
    ///Ex: msockaddr addr=cf->get_tcpaddr("addrParamName","192.168.0.1:2000")
    virtual std::set<msockaddr_in> get_tcpaddr(const std::string&name, const std::string& defv, const std::string& comment)=0;
    virtual std::set<msockaddr_in> get_tcpaddr2(const std::string&name, const std::string& defv, const std::string& comment)=0;

    /// Ex: msockaddr addr=cf->get_tcpaddr("addrParamName","192.168.0.1:2000")
    virtual std::string get_string(const std::string&name, const std::string& defv, const std::string& comment)=0;
    virtual std::string get_string2(const std::string&name, const std::string& defv, const std::string& comment)=0;

    /// Ex: long n=cf->get_xxxx("ParamName",10)
    virtual int64_t get_int64_t(const std::string&name, int64_t defv, const std::string& comment)=0;
    virtual int64_t get_int64_t2(const std::string&name, int64_t defv, const std::string& comment)=0;
    virtual real get_real(const std::string&name, const real& defv, const std::string& comment)=0;
    virtual real get_real2(const std::string&name, const real& defv, const std::string& comment)=0;

    /// Ex: bool n=cf->get_bool("ParamName",true); 2rd param mb 'true'  "false"
    virtual bool get_bool(const std::string&name, bool defv, const std::string& comment)=0;
    virtual bool get_bool2(const std::string&name, bool defv, const std::string& comment)=0;

    /// get std::set<std::string>. Ex: std::set<std::string> n=cf->get_stringset("ParamName","a,b,c");
    virtual std::set<std::string> get_stringset(const std::string&name, const std::string& defv, const std::string& comment)=0;
    virtual std::set<std::string> get_stringset2(const std::string&name, const std::string& defv, const std::string& comment)=0;


    /// print not referenced items to find not used
    // virtual void printUnusedItems()=0;

    virtual void push_prefix(const std::string& p)=0;
    virtual void pop_prefix()=0;
    virtual ~IConfigObj() {}

};
/**
*       Stack prefixer
*       Usage: CFG_PUSH("MyObject",cfg)
*       Into params names the path .MyObject will be added
*/
class st_configObjPrefix
{
    IConfigObj* ref;
public:
    st_configObjPrefix(const std::string& part,IConfigObj* cfg)
        :ref(cfg)
    {
        ref->push_prefix(part);

    }
    ~st_configObjPrefix()
    {
        ref->pop_prefix();
    }

};

#define CFG_PUSH(a,b) st_configObjPrefix st_$$_dummy_obj$pushed(a,b)

