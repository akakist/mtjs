#pragma once
#include "ioBuffer.h"
#include <stdio.h>
#include "base16.h"


///  wrapper for int for debug

struct PK_id
{
    std::string container;
    PK_id() {}
    std::string str() const
    {
        return base16::encode(container).substr(0,4);
    }

};
inline int operator<(const PK_id&a, const PK_id&b)
{
    return a.container<b.container;
}
inline int operator==(const PK_id&a, const PK_id&b)
{
    return a.container==b.container;
}
inline int operator!=(const PK_id&a, const PK_id&b)
{
    return a.container!=b.container;
}
inline outBuffer & operator<< (outBuffer& b,const PK_id &s)
{
    b<<s.container;
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  PK_id &s)
{
    b>>s.container;
    return b;
}



