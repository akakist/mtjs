#include "EVENT_id.h"
#include "IUtils.h"
#include "st_rsa_impl.h"

void registerSSL(const char* pn)
{

    if(pn)
    {
        iUtils->registerPlugingInfo(pn,IUtils::PLUGIN_TYPE_IFACE,Ifaces::SSL,"SSL",std::set<EVENT_id>());
    }
    else
    {
        I_ssl *ssl=new C_ssl;
        iUtils->registerIface(Ifaces::SSL,ssl);
    }
}
