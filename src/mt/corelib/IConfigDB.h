#pragma once
#include <set>
#include <string>
#include <msockaddr_in.h>
#include <sys/types.h>

///       Interface for simple configs


class IConfigDB
{
public:
    virtual msockaddr_in get_tcpaddr(const std::string&name, const std::string& defv)const =0;
    virtual uint64_t get_uint64_t(const std::string&name, const  uint64_t& defv)const =0;
    virtual std::string get_string(const std::string&name, const std::string& defv)const =0;
    virtual void set(const std::string&, const std::string&v)=0;
    virtual ~IConfigDB() {}

};

