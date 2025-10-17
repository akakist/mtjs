#include "geoip_impl.h"
#ifdef _WIN32
#include <malloc.h>
#endif

bool C_GEOIP::findNetRec(const std::string &ip, bool isV4, geoNetRec &result)
{
    if(!inited)
    {
        geoip.init();
        inited=true;
    }
    return geoip.findNetRec(ip,isV4,result);
}
void C_GEOIP::init()
{
    if(!inited)
    {
        geoip.init();
        inited=true;
    }

}

