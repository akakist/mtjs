#include "IUtils.h"
#include "geoip_impl.h"

void registerGEOIP(const char* pn)
{

    if(pn)
    {
        iUtils->registerPlugingInfo(pn,IUtils::PLUGIN_TYPE_IFACE,Ifaces::GEOIP,"GEOIP",std::set<EVENT_id>());
    }
    else
    {
        I_GEOIP *GEOIP=new C_GEOIP;
        iUtils->registerIface(Ifaces::GEOIP,GEOIP);
    }
}
