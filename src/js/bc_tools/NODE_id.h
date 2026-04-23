#pragma once
#include "ioBuffer.h"
#include <stdio.h>


///  wrapper for int for debug

struct NODE_id
{
    std::string container;
    NODE_id() {}
    std::string str() const
    {
        return iUtils->bin2hex(container).substr(0,4);
    }

};
inline int operator<(const NODE_id&a, const NODE_id&b)
{
    return a.container<b.container;
}
inline int operator==(const NODE_id&a, const NODE_id&b)
{
    return a.container==b.container;
}
inline int operator!=(const NODE_id&a, const NODE_id&b)
{
    return a.container!=b.container;
}
inline outBuffer & operator<< (outBuffer& b,const NODE_id &s)
{
    b<<s.container;
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  NODE_id &s)
{
    b>>s.container;
    return b;
}



