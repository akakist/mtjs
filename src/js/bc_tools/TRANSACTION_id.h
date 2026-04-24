#pragma once
#include "ioBuffer.h"
#include <stdio.h>


///  wrapper for int for debug

struct TRANSACTION_body
{
    std::string container;
    TRANSACTION_body() {}
    std::string str() const
    {
        return base62::encode(container).substr(0,4);
    }

};
inline int operator<(const TRANSACTION_body&a, const TRANSACTION_body&b)
{
    return a.container<b.container;
}
inline int operator==(const TRANSACTION_body&a, const TRANSACTION_body&b)
{
    return a.container==b.container;
}
inline int operator!=(const TRANSACTION_body&a, const TRANSACTION_body&b)
{
    return a.container!=b.container;
}
inline outBuffer & operator<< (outBuffer& b,const TRANSACTION_body &s)
{
    b<<s.container;
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  TRANSACTION_body &s)
{
    b>>s.container;
    return b;
}



