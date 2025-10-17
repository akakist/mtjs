#include "CapsAlgorithm.h"
#include "math.h"
#include "mutexInspector.h"
CapsAlgorithm::CapsAlgorithm()
{

}
void CapsAlgorithm::addReferrer(int64_t lat,int64_t lon, const msockaddr_in& sa, int downlinkCount, const msockaddr_in& uplink)
{
    MUTEX_INSPECTOR;
    if(rcontainers.size()>2)
        rcontainers.erase(rcontainers.begin());

    rcontainers[time(nullptr)/3600][sa.family()].add(lat,lon,sa,downlinkCount,uplink);
    rcontainers[time(nullptr)/3600+1][sa.family()].add(lat,lon,sa,downlinkCount,uplink);
}
std::vector<REF_getter<referrerItem> > CapsAlgorithm::findReferrers(int64_t lat,int64_t lon, int addressFamily)
{
    MUTEX_INSPECTOR;
    referrerContainer& c=rcontainers[time(nullptr)/3600][addressFamily];

    iLatLon ll=std::make_pair(int(lat),int(lon));
    iLatLon ll2=std::make_pair(int(lat/2),int(lon/2));
    iLatLon ll4=std::make_pair(int(lat/4),int(lon/4));
    iLatLon ll8=std::make_pair(int(lat/8),int(lon/8));
    std::set<REF_getter<referrerItem> > lookFor;
    if(c.quadratsDegree[ll].size()>MAX_REFFERRERS_COUNT_IN_RESPONSE)
        lookFor=c.quadratsDegree[ll];
    else if(c.quadratsDegree_2[ll2].size()>MAX_REFFERRERS_COUNT_IN_RESPONSE)
        lookFor=c.quadratsDegree_2[ll2];
    else if(c.quadratsDegree_4[ll4].size()>MAX_REFFERRERS_COUNT_IN_RESPONSE)
        lookFor=c.quadratsDegree_4[ll4];
    else if(c.quadratsDegree_8[ll8].size()>MAX_REFFERRERS_COUNT_IN_RESPONSE)
        lookFor=c.quadratsDegree_8[ll8];
    else
    {
        for(auto& z: c.referrers)
            lookFor.insert(z.second);

    }



    std::map<int64_t,std::set<REF_getter<referrerItem> > > dists;
    for(auto& z: lookFor)
    {
        int64_t dist= static_cast<int64_t>(pow(lat - z->lat, 2) + pow(lon - z->lon, 2));
        if(dists.size()>MAX_REFFERRERS_COUNT_IN_RESPONSE && dist>dists.rbegin()->first)
            continue;
        dists[dist].insert(z);

    }
    std::vector<REF_getter<referrerItem> >ret;
    for(auto&z :dists)
    {
        for(auto& x:z.second)
        {
            ret.push_back(x);
            if(ret.size()>MAX_REFFERRERS_COUNT_IN_RESPONSE)
                break;
        }
        if(ret.size()>MAX_REFFERRERS_COUNT_IN_RESPONSE)
            break;
    }
    if(ret.empty())
    {
        for(auto& z:rcontainers[time(nullptr)/3600][addressFamily].referrers)
        {
            ret.push_back(z.second);
            if(ret.size()>MAX_REFFERRERS_COUNT_IN_RESPONSE)
                break;
        }
    }
    return ret;
}
