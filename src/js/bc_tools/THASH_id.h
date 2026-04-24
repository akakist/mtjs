#pragma once
#include "ioBuffer.h"
#include <stdio.h>
// #include "IUtils.h"
#include "base62.h"


///  wrapper for int for debug

struct THASH_id
{
    std::string container;
    THASH_id() {}
    std::string str() const
    {
        return base62::encode(container).substr(0,4);
    }

};
inline int operator<(const THASH_id&a, const THASH_id&b)
{
    return a.container<b.container;
}
inline int operator==(const THASH_id&a, const THASH_id&b)
{
    return a.container==b.container;
}
inline int operator!=(const THASH_id&a, const THASH_id&b)
{
    return a.container!=b.container;
}
inline outBuffer & operator<< (outBuffer& b,const THASH_id &s)
{
    b<<s.container;
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  THASH_id &s)
{
    b>>s.container;
    return b;
}



