#pragma once
#include "ioBuffer.h"
#include <stdio.h>
#include "base16.h"


///  wrapper for int for debug

struct CONTRACT_DATA_id
{
    std::string container;
    CONTRACT_DATA_id() {}

};
inline int operator<(const CONTRACT_DATA_id&a, const CONTRACT_DATA_id&b)
{
    return a.container<b.container;
}
inline int operator==(const CONTRACT_DATA_id&a, const CONTRACT_DATA_id&b)
{
    return a.container==b.container;
}
inline int operator!=(const CONTRACT_DATA_id&a, const CONTRACT_DATA_id&b)
{
    return a.container!=b.container;
}
inline outBuffer & operator<< (outBuffer& b,const CONTRACT_DATA_id &s)
{
    b<<s.container;
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  CONTRACT_DATA_id &s)
{
    b>>s.container;
    return b;
}



