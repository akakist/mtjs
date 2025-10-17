#pragma once
#include <stdint.h>
#include "ifacesEvent.h"
#include <string>
#include <sys/types.h>
#include "ioBuffer.h"
/// interfaces/wrapper to access OpenGEOIP

#define LOCATION_LATITUDE_UNDEF 100000
#define LOCATION_LONGITUDE_UNDEF 100000
struct geoNetRec
{
    std::string startIp,endIp;
    int64_t lat,lon;
};
inline outBuffer & operator<< (outBuffer& b,const geoNetRec &s)
{
    b<<s.startIp<<s.endIp<<s.lat<<s.lon;
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  geoNetRec &s)
{
    b>>s.startIp>>s.endIp>>s.lat>>s.lon;
    return b;
}

inline int operator<(const geoNetRec& a,const geoNetRec& b)
{
    return a.endIp<b.endIp;
}


class I_GEOIP: public Ifaces::Base
{
public:
    virtual void init()=0;
    virtual bool findNetRec(const std::string &ip, bool isV4, geoNetRec &result)=0;

};

