#include "CapsGeoIP.h"
#include <iostream>
#include <fstream>
#include <ios>
#include <math.h>
//#include <sys/unistd.h>
#include <unistd.h>
#include "IUtils.h"
#include "mutexInspector.h"
#include "commonError.h"


inline bool exists_file (const std::string& name) {
    return ( access( name.c_str(), F_OK ) != -1 );
}
std::pair<std::string,std::string> CapsGeoIP::getIpRange(const std::string& ipmask, bool isV4)
{
    MUTEX_INSPECTOR;
    std::vector<std::string> v=iUtils->splitString("/",ipmask);
    std::string ip=v[0];
    if(v.size()!=2)
    {
        logErr2("invalid format");
    }
    int mask=atoi(v[1].c_str());
    if(isV4)
    {
        std::vector<std::string> vi=iUtils->splitString(".",ip);
        unsigned char ipBin[4];
        for(int i=0; i<4; i++)
        {
            ipBin[i]=atoi(vi[i].c_str());
        }
        std::string begin_addr=setBits(ipBin,4,mask,false);
        std::string end_addr=setBits(ipBin,4,mask,true);
        return std::make_pair(begin_addr,end_addr);
    }
    else
    {
        in6_addr addr;
        int r;
#ifdef _WIN32
#if (_WIN32_WINNT >= 0x0600)
        r=inet_pton(AF_INET6, ip.c_str(), &addr);

#else
        throw CommonError("SDK version too small");
#endif
#else
        r=inet_pton(AF_INET6, ip.c_str(), &addr);

#endif
//        r=inet_pton(AF_INET6, ip.c_str(), &addr);
        if(r<0)
            throw CommonError("Not in presentation format %s",ip.c_str());
        unsigned char ipBin[16];
        for(int i=0; i<16; i++)
        {
            ipBin[i]=addr.s6_addr[i];
        }
        std::string begin_addr=setBits(ipBin,16,mask,false);
        std::string end_addr=setBits(ipBin,16,mask,true);
        return std::make_pair(begin_addr,end_addr);
    }
}

void CapsGeoIP::init()
{
#ifndef NO_FILES
    MUTEX_INSPECTOR;
    M_LOCK(this);


    logErr2("CapsGeoIP::init start");


    std::string fileBody;
    std::string fileName="geonets.bin";

    int res=iUtils->load_file_from_disk(fileBody,fileName);
    if(res==-1)
    {
        int idx=0;
        {
            std::string fileName="GeoLite2-City-Blocks-IPv4.csv";
            std::ifstream inF(fileName, std::ios_base::in);

            if(!inF)
            {
                std::cerr << "Cannot open the File : "<<fileName<<std::endl;
                iUtils->setTerminate();
            }

            std::string str;
            // Read the next line from File untill it reaches the end.
            while (std::getline(inF, str))
            {
                idx++;
                if(idx%100000==0)
                    logErr2("%d processed ",idx);
                std::vector<std::string> v=iUtils->splitString(",",str);
                if(v.size()>=3)
                {
                    if(v[0]!="network")
                    {
                        std::string network=v[0];
                        double lat=atof(v[v.size()-3].c_str());
                        double lon=atof(v[v.size()-2].c_str());
                        auto res=getIpRange(network,true);
                        geoNetRec n;
                        n.startIp=res.first;
                        n.endIp=res.second;
                        n.lat=lat*10000;
                        n.lon=lon*10000;
                        geoNets.push_back(n);
                    }
                }

            }
            inF.close();
        }
        {
            std::string fileName="GeoLite2-City-Blocks-IPv6.csv";
            std::ifstream inF(fileName, std::ios_base::in);

            if(!inF)
            {
                std::cerr << "Cannot open the File : "<<fileName<<std::endl;
                iUtils->setTerminate();
            }

            std::string str;
            // Read the next line from File untill it reaches the end.
            while (std::getline(inF, str))
            {
                idx++;
                if(idx%100000==0)
                    logErr2("%d processed ",idx);
                std::vector<std::string> v=iUtils->splitString(",",str);
                if(v.size()>=3)
                {
                    if(v[0]!="network")
                    {
                        std::string network=v[0];
                        double lat=atof(v[v.size()-3].c_str());
                        double lon=atof(v[v.size()-2].c_str());
                        auto res=getIpRange(network,false);
                        geoNetRec n;
                        n.startIp=res.first;
                        n.endIp=res.second;
                        n.lat=lat*10000;
                        n.lon=lon*10000;
                        geoNets.push_back(n);
                    }
                }
            }
            inF.close();
        }
        logErr2("start sort");
        sort(geoNets.begin(),geoNets.end(),cmpGeo);
        logErr2("end sort");
        outBuffer o;
        o<<geoNets;
        REF_getter<refbuffer> b=o.asString();
        FILE *f=fopen(fileName.c_str(),"wb");
        size_t r=fwrite(b->buffer,1,b->size_,f);
        if(r!=b->size_)
            throw CommonError("if(r!=b->size_)");
        fclose(f);
    }
    else
    {
        inBuffer in(fileBody);
        in>>geoNets;
    }
    logErr2("CapsGeoIP::init end");
#endif
}
int CapsGeoIP::cmpGeo(const geoNetRec&a,const geoNetRec &b)
{
    MUTEX_INSPECTOR;
    return a.endIp<b.endIp;
}

CapsGeoIP::CapsGeoIP()
{
}
bool CapsGeoIP::findNetRec(const std::string &ip, bool isV4,geoNetRec& result)
{
    MUTEX_INSPECTOR;

    std::string ipBin=getIpBin(ip,isV4);

    geoNetRec sample;
    sample.endIp=ipBin;

    auto it=std::lower_bound(geoNets.begin(),geoNets.end(),sample);
    if(it!=geoNets.end())
    {
        result=*it;
        return true;
    }
    return false;
}

std::string CapsGeoIP::setBits(unsigned char* _ip, int iplen, int netmask, bool value)
{
    MUTEX_INSPECTOR;
    unsigned char ip[iplen];
    memcpy(ip,_ip,iplen);
    for(int i=netmask; i<iplen*8; i++)
    {
        int nbyte=i/8;
        int nbit=i%8;
        if(value)
        {
            ip[nbyte]|=1<<(7-nbit);
        }
        else
        {
            ip[nbyte]&=~(1<<(7-nbit));
        }
    }
    return std::string((char*)ip,iplen);
}
std::string CapsGeoIP::getIpBin(const std::string& ip,  bool isV4)
{
    MUTEX_INSPECTOR;
    if(isV4)
    {
        std::vector<std::string> vi=iUtils->splitString(".",ip);
        unsigned char ipBin[4];
        for(int i=0; i<4; i++)
        {
            ipBin[i]=atoi(vi[i].c_str());
        }
        return std::string((char*)ipBin,4);
    }
    else
    {
        in6_addr addr;
        int r;
#ifdef _WIN32
#if (_WIN32_WINNT >= 0x0600)
        r=inet_pton(AF_INET6, ip.c_str(), &addr);
#else
        throw CommonError("SDK version too small");
#endif
#else
        r=inet_pton(AF_INET6, ip.c_str(), &addr);
#endif

//        r=inet_pton(AF_INET6, ip.c_str(), &addr);
        if(r<0)
            throw CommonError("Not in presentation format %s",ip.c_str());
        unsigned char ipBin[16];
        for(int i=0; i<16; i++)
        {
            ipBin[i]=addr.s6_addr[i];
        }
        return std::string((char*)ipBin,16);
    }

}
bool CapsGeoIP::getFileContent(std::string fileName, std::vector<std::string> & vecOfStrs)
{
    MUTEX_INSPECTOR;

    // Open the File
    std::ifstream inF(fileName.c_str(), std::ios_base::in);

    // Check if object is valid
    if(!inF)
    {
        std::cerr << "Cannot open the File : "<<fileName<<std::endl;
        return false;
    }

    std::string str;
    // Read the next line from File untill it reaches the end.
    while (std::getline(inF, str))
    {
        // Line contains string of length > 0 then save it in vector
        if(str.size() > 0)
            vecOfStrs.push_back(str);
    }
    //Close The File
    inF.close();
    return true;
}
