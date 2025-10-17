#pragma once

#include <IGEOIP.h>
#include "CapsGeoIP.h"

class C_GEOIP: public I_GEOIP
{
public:
    bool inited;
    CapsGeoIP geoip;

    bool findNetRec(const std::string &ip, bool isV4, geoNetRec &result);
    void init();

    C_GEOIP():inited(false)
    {

    }

};



