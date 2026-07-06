#pragma once
#include "msockaddr_in.h"

class IRPC
{
    /// interface to access to RPC service
public:
    virtual unsigned short getExternalListenPortMain()=0; // network byte order
    virtual std::set<msockaddr_in> getInternalListenAddrs()=0; // network byte order

    IRPC() {}
    virtual ~IRPC() {}

};

