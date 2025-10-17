#pragma once
#include <map>
#include "ioBuffer.h"
#include "msockaddr_in.h"
#define MAX_REFFERRERS_COUNT_IN_RESPONSE 40
struct referrerItem: public Refcountable
{
    msockaddr_in sa;
    time_t lastReport;
    int dowlinkCount;
    int64_t lat,lon;
    msockaddr_in uplink;
    void pack(outBuffer& b)
    {
        b<<sa<<(int64_t)lastReport<<dowlinkCount<<lat<<lon<<uplink;
    }
    void unpack(inBuffer& in)
    {
        int64_t lr = 0;
//        in>>sa;
        in>>sa>>lr>>dowlinkCount>>lat>>lon>>uplink;
        lastReport= static_cast<time_t>(lr);
    }
};

typedef std::pair<int,int> iLatLon;

struct referrerContainer
{
    void add(int64_t lat,int64_t lon, const msockaddr_in& sa, int downlinkCount, const msockaddr_in& uplink)
    {
        auto it=referrers.find(sa);
        if(it==referrers.end())
        {
            REF_getter<referrerItem> i=new referrerItem;
            referrers.insert(std::make_pair(sa,i));
        }
        it=referrers.find(sa);
        it->second->dowlinkCount=downlinkCount;
        it->second->lastReport=time(NULL);
        it->second->lat=lat;
        it->second->lon=lon;
        it->second->sa=sa;
        it->second->uplink=uplink;

        iLatLon ll=std::make_pair(int(lat),int(lon));
        iLatLon ll2=std::make_pair(int(lat/2),int(lon/2));
        iLatLon ll4=std::make_pair(int(lat/4),int(lon/4));
        iLatLon ll8=std::make_pair(int(lat/8),int(lon/8));
        quadratsDegree[ll].insert(it->second);
        quadratsDegree_2[ll2].insert(it->second);
        quadratsDegree_4[ll4].insert(it->second);
        quadratsDegree_8[ll8].insert(it->second);

    }
    std::map<msockaddr_in,REF_getter<referrerItem> > referrers;

    std::map<iLatLon,std::set<REF_getter<referrerItem> > > quadratsDegree;
    std::map<iLatLon,std::set<REF_getter<referrerItem> > > quadratsDegree_2;
    std::map<iLatLon,std::set<REF_getter<referrerItem> > > quadratsDegree_4;
    std::map<iLatLon,std::set<REF_getter<referrerItem> > > quadratsDegree_8;

};

class CapsAlgorithm
{
public:

    std::map<time_t,std::map<int/*AF*/,referrerContainer> > rcontainers;

    void addReferrer(int64_t lat, int64_t lon, const msockaddr_in& sa, int downlinkCount, const msockaddr_in &uplink);

    std::vector<REF_getter<referrerItem> > findReferrers(int64_t lat, int64_t lon, int addressFamily);

    CapsAlgorithm();
};

