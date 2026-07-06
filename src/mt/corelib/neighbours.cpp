#include "neighbours.h"

#include "configDB.h"
#include "ioBuffer.h"
#include <sstream>
#define NEIGBOURS_DB_NAME "neighboursz"

_neighbours::_neighbours(const std::string& _dbName):dbName(_dbName) {
    ConfigDB_private c;
    try {
        std::string s = c.get_string("_neighbours."+dbName, "");
        if (s.size())
        {
            inBuffer in(s);
            in>>sas;
        }
    }
    catch(const std::exception& e)
    {
        printf("_neighbours::_neighbours: std::exception: %s",e.what());
    }

}

void _neighbours::remove(const msockaddr_in& sa)
{
    sas.erase(sa);
    outBuffer o;
    o<<sas;
    ConfigDB_private c;
    c.set("_neighbours."+dbName,o.buffer->container);

}

std::vector<msockaddr_in> _neighbours::getAllAndClear()
{
    std::vector<msockaddr_in> ret;
    ret.reserve(sas.size());
    for(auto z: sas)
        ret.push_back(z);
    sas.clear();
    outBuffer o;
    o<<sas;
    ConfigDB_private c;
    c.set("_neighbours."+dbName,o.buffer->container);
    return ret;
}
void _neighbours::add(const msockaddr_in &sa, int pingTime)
{

    sas.insert(sa);
    outBuffer o;
    o<<sas;
    ConfigDB_private c;
    c.set("_neighbours."+dbName,o.buffer->container);

}
