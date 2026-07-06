#include "configDB.h"
#ifdef _WIN32
#include "compat_win32.h"
#endif

#include "st_FILE.h"


ConfigDB::ConfigDB(bool shared):shared_(shared)
{

}

std::string ConfigDB::val(const std::string& key) const
{
    MUTEX_INSPECTOR;
    if(iUtils->calcFileSize(fileName(key))!=-1)
        return iUtils->load_file_no_throw(fileName(key));
    return "";
}


msockaddr_in ConfigDB::get_tcpaddr(const std::string&name, const std::string& defv)const
{
    MUTEX_INSPECTOR;
    XTRY;
    std::string v=val(name);
    if(v.empty())
    {
        v=defv;
    }
    // Url u;
    // u.parse(v);
    // if(u.host=="INADDR_ANY")
    // {

    // }

    msockaddr_in bindAddr;

    bindAddr.init(v);
    return bindAddr;
    XPASS;

}

std::string ConfigDB::get_string(const std::string& name, const std::string& defv) const
{
    std::string v=val(name);
    if(v.empty())
    {
        v=defv;
    }
    return v;
}
uint64_t ConfigDB::get_uint64_t(const std::string&name, const uint64_t& defv)const
{
    std::string vv=val(name);
    if(vv.size()==0)
    {
        return defv;
    }
#ifdef _MSC_VER
    return atol(vv.c_str());
#else
    return static_cast<uint64_t>(atoll(vv.c_str()));
#endif
}
std::string ConfigDB::fileName(const std::string &k) const
{
    std::string baseDir;

    baseDir=iUtils->gCacheDir();

    std::string pn;
    if(shared_)
        pn=baseDir+"/config."+k;
    else
        pn=baseDir+ "/config."+iUtils->app_name()+"."+k;
    return pn;

}

void ConfigDB::set(const std::string&k, const std::string&v)
{
    MUTEX_INSPECTOR;
    std::string pn=fileName(k);
    if(!v.empty())
    {
        st_FILE f(pn.c_str(),"wb");
        fwrite(v.data(),1,v.size(),f.f);
    }
    else
    {
        unlink(pn.c_str());
    }

}
