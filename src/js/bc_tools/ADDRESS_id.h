#pragma once
#include "ioBuffer.h"
#include <stdio.h>
#include "base16.h"


///  wrapper for int for debug

struct ADDRESS_id
{
    std::string addr;
    ADDRESS_id() {}
    std::string str() const
    {
        return base16::encode(addr).substr(0,4);
    }

};
inline int operator<(const ADDRESS_id&a, const ADDRESS_id&b)
{
    return a.addr<b.addr;
}
inline int operator==(const ADDRESS_id&a, const ADDRESS_id&b)
{
    return a.addr==b.addr;
}
inline int operator!=(const ADDRESS_id&a, const ADDRESS_id&b)
{
    return a.addr!=b.addr;
}
inline outBuffer & operator<< (outBuffer& b,const ADDRESS_id &s)
{
    b<<s.addr;
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  ADDRESS_id &s)
{
    b>>s.addr;
    return b;
}



