#include "msockaddr_in.h"
#include "IUtils.h"
#ifndef _WIN32
#include <netdb.h>
#ifdef __linux__
#include <linux/in6.h>
#endif

#endif
#ifdef _WIN32
#include <ws2tcpip.h>
#endif
#include "commonError.h"
#include "mutexInspector.h"
#include <sys/un.h>
#include <string.h>

outBuffer & operator<<(outBuffer&b,const msockaddr_in &a)
{
    return a.pack(b);
}
inBuffer & operator>>(inBuffer &b,msockaddr_in &a)
{
    return a.unpack(b);
}

bool msockaddr_in::operator<(const msockaddr_in&b) const
{
    MUTEX_INSPECTOR;
    if(family()<b.family()) return true;
    if(family()>b.family()) return false;

    if(family()==AF_UNIX)
    {
        return strcmp(u.sa_un.sun_path,b.u.sa_un.sun_path);
    }
    else if(family()==AF_INET)
    {
        if (u.sa4.sin_addr.s_addr<b.u.sa4.sin_addr.s_addr) return true;
        if (u.sa4.sin_addr.s_addr>b.u.sa4.sin_addr.s_addr) return false;
        if (u.sa4.sin_port<b.u.sa4.sin_port) return true;
        if (u.sa4.sin_port>b.u.sa4.sin_port) return false;
        return false;
    }
    else if(family()==AF_INET6)
    {
        for(size_t i=0; i<sizeof(u.sa6.sin6_addr.s6_addr); i++)
        {
            if (u.sa6.sin6_addr.s6_addr[i]<b.u.sa6.sin6_addr.s6_addr[i]) return true;
            if (u.sa6.sin6_addr.s6_addr[i]>b.u.sa6.sin6_addr.s6_addr[i]) return false;
        }
//#elif defined(__ANDROID__)
//        for(size_t i=0; i<sizeof(u.sa6.sin6_addr.in6_u.u6_addr8); i++)
//        {
//            if (u.sa6.sin6_addr.in6_u.u6_addr8[i]<b.u.sa6.sin6_addr.in6_u.u6_addr8[i]) return true;
//            if (u.sa6.sin6_addr.in6_u.u6_addr8[i]>b.u.sa6.sin6_addr.in6_u.u6_addr8[i]) return false;
//        }
//#elif defined __FreeBSD__

//        for(size_t i=0; i<sizeof(u.sa6.sin6_addr.__u6_addr.__u6_addr8); i++)
//        {
//            if (u.sa6.sin6_addr.__u6_addr.__u6_addr8[i]<b.u.sa6.sin6_addr.__u6_addr.__u6_addr8[i]) return true;
//            if (u.sa6.sin6_addr.__u6_addr.__u6_addr8[i]>b.u.sa6.sin6_addr.__u6_addr.__u6_addr8[i]) return false;
//        }

//#elif defined _WIN32
//        for(size_t i=0; i<sizeof(u.sa6.sin6_addr.u.Byte); i++)
//        {
//            if (u.sa6.sin6_addr.u.Byte[i]<b.u.sa6.sin6_addr.u.Byte[i]) return true;
//            if (u.sa6.sin6_addr.u.Byte[i]>b.u.sa6.sin6_addr.u.Byte[i]) return false;
//        }
//#else

//        for(size_t i=0; i<sizeof(u.sa6.sin6_addr.__in6_u.__u6_addr8); i++)
//        {
//            if (u.sa6.sin6_addr.__in6_u.__u6_addr8[i]<b.u.sa6.sin6_addr.__in6_u.__u6_addr8[i]) return true;
//            if (u.sa6.sin6_addr.__in6_u.__u6_addr8[i]>b.u.sa6.sin6_addr.__in6_u.__u6_addr8[i]) return false;
//        }

//#endif
        if (u.sa6.sin6_port<b.u.sa6.sin6_port) return true;
        if (u.sa6.sin6_port>b.u.sa6.sin6_port) return false;
        return false;
    }
    else
    {
        throw CommonError("1 invalid family %s %s",family(),_DMI().c_str());
    }

    return false;
}
bool msockaddr_in::operator==(const msockaddr_in&b) const
{
    if(family()!=b.family()) return false;
    if(family()==AF_UNIX)
    {
        return !strcmp(u.sa_un.sun_path,b.u.sa_un.sun_path);
    }
    else if(family()==AF_INET)
    {
        if(u.sa4.sin_addr.s_addr!=b.u.sa4.sin_addr.s_addr) return false;
        if(u.sa4.sin_port!=b.u.sa4.sin_port) return false;
    }
    else if(family()==AF_INET6)
    {
        if(u.sa6.sin6_port != b.u.sa6.sin6_port) return false;
//#ifdef __MACH__
        for(size_t i=0; i<sizeof(u.sa6.sin6_addr.s6_addr); i++)
        {
            if (u.sa6.sin6_addr.s6_addr[i]!=b.u.sa6.sin6_addr.s6_addr[i])
                return false;
        }
//#elif defined(__ANDROID__)
//        for(size_t i=0; i<sizeof(u.sa6.sin6_addr.in6_u.u6_addr8); i++)
//        {
//            if (u.sa6.sin6_addr.in6_u.u6_addr8[i]!=b.u.sa6.sin6_addr.in6_u.u6_addr8[i]) return false;
//        }
//#elif defined __FreeBSD__
//        for(size_t i=0; i<sizeof(u.sa6.sin6_addr.__u6_addr.__u6_addr8); i++)
//        {
//            if (u.sa6.sin6_addr.__u6_addr.__u6_addr8[i]!=b.u.sa6.sin6_addr.__u6_addr.__u6_addr8[i]) return false;
//        }

//#elif defined _WIN32
//        for(size_t i=0; i<sizeof(u.sa6.sin6_addr.u.Byte); i++)
//        {
//            if (u.sa6.sin6_addr.u.Byte[i]!=b.u.sa6.sin6_addr.u.Byte[i]) return false;
//        }

//#else
//        for(size_t i=0; i<sizeof(u.sa6.sin6_addr.__in6_u.__u6_addr8); i++)
//        {
//            if (u.sa6.sin6_addr.__in6_u.__u6_addr8[i]!=b.u.sa6.sin6_addr.__in6_u.__u6_addr8[i]) return false;
//        }

//#endif

    }
    else
        throw CommonError("3 invalid family %d %s",family(),_DMI().c_str());
    return true;
}

outBuffer &msockaddr_in::pack(outBuffer &b) const
{
    if(family()==AF_UNIX)
    {
        b.put_PN(u.sa_un.sun_family);
        b.put_PSTR(u.sa_un.sun_path);

    }
    else if(family()==AF_INET)
    {
        b.put_PN(u.sa4.sin_family);
        b.put_PN(ntohs(u.sa4.sin_port));
        b.put_PN(ntohl(u.sa4.sin_addr.s_addr));
    }
    else if(family()==AF_INET6)
    {
        b.put_PN(u.sa6.sin6_family);
        b.put_PN(htons(u.sa4.sin_port));
        char str[100];
#ifdef _WIN32
#if (_WIN32_WINNT >= 0x0600)
        InetNtopA(u.sa6.sin6_family,& u.sa6.sin6_addr,str,sizeof(str));
#else
        throw CommonError("windows version too old");
#endif
#else
        inet_ntop(u.sa6.sin6_family, &u.sa6.sin6_addr, str,sizeof(str));
#endif
        b.put_PSTR(str);
    }
    else
        throw CommonError("3 invalid family %d %s",family(),_DMI().c_str());


    return b;
}
inBuffer &msockaddr_in::unpack(inBuffer&b)
{

    MUTEX_INSPECTOR;
    uint8_t fam=b.get_PN();
    if(fam==AF_UNIX)
    {
        auto s=b.get_PSTR();
        u.sa_un.sun_family=fam;
        strcpy(u.sa_un.sun_path,s.c_str());
    }
    else if(fam==AF_INET)
    {
        u.sa4.sin_family=fam;
        u.sa4.sin_port=htons(b.get_PN());
        u.sa4.sin_addr.s_addr=htonl(b.get_PN());
    }
    else if(fam==AF_INET6)
    {
        u.sa6.sin6_family=fam;
        u.sa6.sin6_port=htons(b.get_PN());
        std::string s=b.get_PSTR();
#ifdef _WIN32
#if (_WIN32_WINNT >= 0x0600)
        inet_pton(u.sa6.sin6_family,s.c_str(),&u.sa6.sin6_addr);
#else
        throw CommonError("SDK version too small");
#endif
#else
        inet_pton(u.sa6.sin6_family,s.c_str(),&u.sa6.sin6_addr);
#endif
    }
    else
        throw CommonError("2 invalid family %d %s",fam,_DMI().c_str());
    return b;
}
std::string msockaddr_in::getStringAddr() const
{
    MUTEX_INSPECTOR;
    if(family()==AF_UNIX)
    {
        return u.sa_un.sun_path;
    }
    else if(family()==AF_INET)
    {
        return inet_ntoa(u.sa4.sin_addr);
    }
    else if(family()==AF_INET6)
    {
        char str[100];
#ifdef _WIN32
#if (_WIN32_WINNT >= 0x0600)
        inet_ntop(u.sa6.sin6_family, &u.sa6.sin6_addr, str,sizeof(str));
#else
        throw CommonError("SDK version too small");
#endif
#else
        inet_ntop(u.sa6.sin6_family, &u.sa6.sin6_addr, str,sizeof(str));
#endif
        return str;
    }
    else
        throw CommonError("3 invalid family %d %s",family(),_DMI().c_str());
}
unsigned short msockaddr_in::port() const
{
    MUTEX_INSPECTOR;
    if(family()==AF_UNIX)
    {
        throw CommonError("invalid call %s",__FUNCTION__);
    }
    if(family()==AF_INET)
        return ntohs(u.sa4.sin_port);
    else if(family()==AF_INET6)
        return ntohs(u.sa6.sin6_port);
    else
        throw CommonError("3 invalid family %d %s",family(),_DMI().c_str());
}

msockaddr_in::msockaddr_in(const msockaddr_in& n)
{
    memset(&u,0,sizeof(u));

    memcpy(&u,&n.u,sizeof(n.u));
}

std::string msockaddr_in::dump() const
{
    char s[200];
    if(family()==AF_UNIX)
    {
        snprintf(s,sizeof(s),"unix@%s",u.sa_un.sun_path);
    }
    else if(family()==AF_INET)
    {
        snprintf(s,sizeof(s),"%s:%d",getStringAddr().c_str(), ntohs(u.sa4.sin_port));
    }
    else if(family()==AF_INET6)
    {
        snprintf(s,sizeof(s),"[%s]:%d",getStringAddr().c_str(), ntohs(u.sa4.sin_port));
    }
    else
        throw CommonError("3 invalid family %d %s",family(),_DMI().c_str());

    return s;

}

std::pair<std::set<std::string>,std::set<std::string> > msockaddr_in::getAddrInfo(const std::string& host)
{
    MUTEX_INSPECTOR;
    auto ai=iUtils->getAddrInfos();
    {
        R_LOCK(ai.get()->lk);

        auto pa=ai->host2AddrInfo.find(host);
        if(pa!=ai->host2AddrInfo.end())
        {
            if(time(NULL)-pa->second.t<4*3600)
            {
                return pa->second.addrs;
            }
        }
    }

    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET6_ADDRSTRLEN];



    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // AF_INET или AF_INET6 для указания версии протокола
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(host.c_str(), NULL, &hints, &res)) != 0) {
        throw CommonError("getaddrinfo: %s\n%s", gai_strerror(status),host.c_str());
    }

    std::set<std::string> ipv4;
    std::set<std::string> ipv6;

    for(p = res; p != NULL; p = p->ai_next) {
        void *addr;
        std::string ipver;


        // получаем указатель на адрес, по разному в разных протоколах
        if (p->ai_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } else { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            for(int i=0; i<16; i++)
            {
#ifdef __MACH__

#elif defined (_WIN32)
#else
#endif

            }
            ipver = "IPv6";
        }

        // преобразуем IP в строку и выводим его:
#ifdef _WIN32
#if (_WIN32_WINNT >= 0x0600)
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
#else
        throw CommonError("SDK version too small");
#endif
#else
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
#endif

        if(p->ai_family == AF_INET)
            ipv4.insert(ipstr);
        else
            ipv6.insert(ipstr);

    }

    freeaddrinfo(res); // free the linked list

    {
        W_LOCK(ai.get()->lk);
        ai->host2AddrInfo.erase(host);
        ai->host2AddrInfo.insert(std::make_pair(host,_addrInfo(std::make_pair(ipv4,ipv6))));

    }
    return std::make_pair(ipv4,ipv6);
}

void msockaddr_in::init(const std::string& host, unsigned short port)
{
    MUTEX_INSPECTOR;
    XTRY;
    if(host=="INADDR_ANY")
    {
#ifdef __MACH__
        u.sa4.sin_len=sizeof(sockaddr_in);
#endif
        u.sa4.sin_family=AF_INET;
        u.sa4.sin_port=htons(port);
        u.sa4.sin_addr.s_addr=INADDR_ANY;
        return;
    }
    else if(host=="INADDR6_ANY")
    {
#ifdef __MACH__
        u.sa6.sin6_len=sizeof(sockaddr_in6);
#endif
        u.sa6.sin6_family=AF_INET6;
        u.sa6.sin6_port=htons(port);
        u.sa6.sin6_addr = in6addr_any;
        return;
    }
    std::pair<std::set<std::string>,std::set<std::string> > z;
    z=getAddrInfo(host);


    if(z.first.size())
    {
#ifdef __MACH__
        u.sa4.sin_len=sizeof(sockaddr_in);
#endif
        u.sa4.sin_family=AF_INET;
        u.sa4.sin_port=htons(port);
        u.sa4.sin_addr.s_addr=inet_addr(z.first.begin()->c_str());
    }
    else if(z.second.size())
    {
#ifdef __MACH__
        u.sa6.sin6_len=sizeof(sockaddr_in6);
#endif
        u.sa6.sin6_family=AF_INET6;
        u.sa6.sin6_port=htons(port);
#ifdef _WIN32
#if (_WIN32_WINNT >= 0x0600)
        inet_pton(u.sa6.sin6_family,z.second.begin()->c_str(),&u.sa6.sin6_addr);
#else
        throw CommonError("SDK version too small");
#endif
#else
        inet_pton(u.sa6.sin6_family,z.second.begin()->c_str(),&u.sa6.sin6_addr);
#endif
    }
    else throw CommonError("invalid case!");
    XPASS;
}
void msockaddr_in::setPort(const unsigned short& port)
{
    if(family()==AF_INET)
        u.sa4.sin_port=htons(port);
    else if(family()==AF_INET6)
        u.sa6.sin6_port=htons(port);
    else if(family()==AF_UNIX)
        throw CommonError("else if(family()==AF_UNIX)");
    else
        throw CommonError("3 invalid family %d",family());

}
void msockaddr_in::init(const std::string& s)
{
    if(s.substr(0,5)=="unix@")
    {
        /// init unix sockadr
        // u.sa_un.sun_len=sizeof(sockaddr_un);
        u.sa_un.sun_family=AF_UNIX;
        auto a=s.substr(5);
        strcpy(u.sa_un.sun_path,a.c_str());
        // printf("u.sa_un.sun_path %s\n",u.sa_un.sun_path);
        // u.sa_un.sun_len=offsetof(struct sockaddr_un, sun_path)+strlen(u.sa_un.sun_path)+1;

    }
    else
    {
        auto pz=s.find(":");
        if(pz!=std::string::npos)
        {
            auto host=s.substr(0,pz);
            auto port=atoi(s.substr(pz+1,s.size()-(pz+1)).c_str());
            init(host.c_str(),port);
        }
        else throw CommonError("invalid case");
    }
}

static bool check_addr_for_mask(in_addr a, const char *ipmask, int maskShift)
{
    unsigned int val=ntohl(a.s_addr);

    int shift=32-maskShift;
    val>>=shift;
    val<<=shift;

    a.s_addr=htonl(val);
    std::string rest=inet_ntoa(a);
    return rest==ipmask;

}
bool msockaddr_in::isNAT() const
{
    if(family()==AF_INET)
    {
        /**
            10.0.0.0/8       (10.0.0.0 - 10.255.255.255)
            172.16.0.0/12    (172.16.0.0 - 172.31.255.255)
            192.168.0.0/16   (192.168.0.0 - 192.168.255.255)

        */
        if(check_addr_for_mask(u.sa4.sin_addr,"10.0.0.0",8))
            return true;
        if(check_addr_for_mask(u.sa4.sin_addr,"172.16.0.0",12))
            return true;
        if(check_addr_for_mask(u.sa4.sin_addr,"192.168.0.0",16))
            return true;
    }

    return false;
}
std::string  msockaddr_in::asString() const
{
    outBuffer o;
    pack(o);
    return std::move(o.buffer->container);
}
void msockaddr_in::initFromUrl(const std::string &url)
{
    // Url u;
    // u.parse(url);
    init(url);
}

socklen_t msockaddr_in::addrLen() const
{
    if(family()==AF_INET)
        return sizeof(sockaddr_in);
    else if(family()==AF_INET6)
        return sizeof(sockaddr_in6);
    else if(family()==AF_UNIX)
        return sizeof(sockaddr_un);
    else
        throw CommonError("3 invalid family %d %s",family(),_DMI().c_str());

}
socklen_t msockaddr_in::maxAddrLen() const
{
    return sizeof(u);
}
sockaddr* msockaddr_in::addr() const
{
    return (sockaddr*)&u;
}
int8_t msockaddr_in::family() const
{
    return u.sa4.sin_family;
}

