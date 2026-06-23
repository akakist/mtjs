#pragma once
#include "ioBuffer.h"
#include <stdio.h>
#include "base16.h"


///  wrapper for int for debug

struct EPOCH_id
{
    uint64_t container;
    EPOCH_id() {}

};
inline int operator<(const EPOCH_id&a, const EPOCH_id&b)
{
    return a.container<b.container;
}
inline int operator>(const EPOCH_id&a, const EPOCH_id&b)
{
    return a.container>b.container;
}
inline int operator==(const EPOCH_id&a, const EPOCH_id&b)
{
    return a.container==b.container;
}
inline int operator!=(const EPOCH_id&a, const EPOCH_id&b)
{
    return a.container!=b.container;
}
inline outBuffer & operator<< (outBuffer& b,const EPOCH_id &s)
{
    b<<s.container;
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  EPOCH_id &s)
{
    b>>s.container;
    return b;
}



