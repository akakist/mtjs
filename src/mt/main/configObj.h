#pragma once

#include <IConfigObj.h>
#include "Real.h"
#include <deque>
class ConfigObj:public IConfigObj, public Mutexable
{
    friend class st_configObjPrefix;
    std::map<std::string,std::pair<std::string, bool> > m_container;
    std::map<pthread_t,std::deque<std::string> >m_prefixes;
    std::string m_filename;
    std::string m_getPath(const std::string&name) const;
    std::string m_getPath2(const std::string&name) const;

    void m_initFromLine(const std::string& buf);
    // void appendLine(const std::string& key, const std::string& val, const std::string& comment);
    // void appendLine(const std::string& key, const bool& val, const std::string& comment);
    // void appendLine(const std::string& key, const real &val, const std::string& comment);
    // void appendLine(const std::string& key, const int64_t& val, const std::string& comment);
    // void appendLine(const std::string& key, const uint64_t& val, const std::string& comment);

public:
    ConfigObj() {}

    ConfigObj(const std::string& __filename):m_filename(__filename)
    {
        // load_from_file();
    }
    ConfigObj(const std::string& fake_filename, const std::string& buffer):m_filename(fake_filename)
    {
        load_from_buffer(buffer);
    }
    // void load_from_file();
    void load_from_buffer(const std::string& buf);
protected:
public:
    std::set<msockaddr_in>get_tcpaddr(const std::string&name, const std::string& defv, const std::string& comment);
    std::set<msockaddr_in>get_tcpaddr2(const std::string&name, const std::string& defv, const std::string& comment);
    std::string get_string(const std::string&name, const std::string& defv, const std::string& comment) ;
    std::string get_string2(const std::string&name, const std::string& defv, const std::string& comment) ;
    int64_t get_int64_t(const std::string&name, int64_t defv, const std::string& comment);
    int64_t get_int64_t2(const std::string&name, int64_t defv, const std::string& comment);
    uint64_t get_uint64_t(const std::string&name, uint64_t defv, const std::string& comment);

    real get_real(const std::string&name, const real &defv, const std::string& comment);
    real get_real2(const std::string&name, const real& defv, const std::string& comment);
    bool get_bool(const std::string&name, bool defv, const std::string& comment) ;
    bool get_bool2(const std::string&name, bool defv, const std::string& comment) ;
    std::set<std::string> get_stringset(const std::string&name, const std::string& defv, const std::string& comment) ;
    std::set<std::string> get_stringset2(const std::string&name, const std::string& defv, const std::string& comment) ;
    void push_prefix(const std::string& p);
    void pop_prefix();
    // void printUnusedItems();


private:
};



