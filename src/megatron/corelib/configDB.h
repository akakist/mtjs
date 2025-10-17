#pragma once
#include "msockaddr_in.h"


/// simple config store

class ConfigDB_shared;
class ConfigDB_private;
class ConfigDB
{
    friend class ConfigDB_private;
    friend class ConfigDB_shared;

    std::string val(const std::string& key) const;
    ConfigDB(bool shared);
    std::string fileName(const std::string &k) const;

public:

    bool shared_;
    void set(const std::string& key, const std::string&val);
    msockaddr_in get_tcpaddr(const std::string&name, const std::string& defv)const;
    uint64_t get_uint64_t(const std::string&name, const uint64_t& defv)const;
    std::string get_string(const std::string&name, const std::string& defv)const;
};
class ConfigDB_shared: public ConfigDB
{
public:
    ConfigDB_shared():ConfigDB(true) {}
};
class ConfigDB_private: public ConfigDB
{
public:
    ConfigDB_private():ConfigDB(false) {}
};

