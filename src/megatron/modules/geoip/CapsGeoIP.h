#pragma once
#include <string>
#include "IGEOIP.h"
#define IPDATA "ipdata.txt"

#define DBNAME "geoip"
#define NO_FILES 1
class CapsGeoIP
{
public:
    CapsGeoIP();
    void init();


    std::vector<geoNetRec> geoNets;

    bool findNetRec(const std::string &ip, bool isV4, geoNetRec &result);
    std::pair<std::string,std::string> getIpRange(const std::string& ipmask, bool isV4);
    static std::string setBits(unsigned char* _ip, int iplen, int netmask, bool value);
    static std::string getIpBin(const std::string& ip,  bool isV4);
    static bool getFileContent(std::string fileName, std::vector<std::string> & vecOfStrs);
    static int cmpGeo(const geoNetRec&a,const geoNetRec &b);

};

