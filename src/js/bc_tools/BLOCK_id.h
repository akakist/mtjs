#pragma once
#include "ioBuffer.h"
#include <stdio.h>


///  wrapper for int for debug

struct BLOCK_id
{
    std::string container;
    BLOCK_id() {}
    std::string str() const
    {
        return iUtils->bin2hex(container).substr(0,4);
    }

};
inline int operator<(const BLOCK_id&a, const BLOCK_id&b)
{
    return a.container<b.container;
}
inline int operator==(const BLOCK_id&a, const BLOCK_id&b)
{
    return a.container==b.container;
}
inline int operator!=(const BLOCK_id&a, const BLOCK_id&b)
{
    return a.container!=b.container;
}
inline outBuffer & operator<< (outBuffer& b,const BLOCK_id &s)
{
    b<<s.container;
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  BLOCK_id &s)
{
    b>>s.container;
    return b;
}



