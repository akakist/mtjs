#include "IUtils.h"
#define _FILE_OFFSET_BITS 64

#include "threadNameCtl.h"
#include "mutex_inspector_entry.h"
#include "commonError.h"
class ThreadNameControllerImpl:public Mutexable
{
public:
    std::map<pthread_t,std::string> container_pt2str;
    std::map<std::string,pthread_t> container_str2pt;
    std::map < pthread_t, std::deque < mutex_inspector_entry > >mutex_inspector_list;
};



void ThreadNameController::add_LK(const pthread_t &key, const std::string& val)
{
    M_LOCK(impl);
    impl->container_pt2str.insert(std::make_pair(key,val));
    impl->container_str2pt.insert(std::make_pair(val,key));
}
void ThreadNameController::remove_LK(const pthread_t&key)
{
    M_LOCK(impl);
    auto i=impl->container_pt2str.find(key);
    if (i==impl->container_pt2str.end()) throw CommonError("ThreadNameController: access to undefined pthread %X (%s %d)",key,__FILE__,__LINE__);
    auto j=impl->container_str2pt.find(i->second);
    if (j==impl->container_str2pt.end()) throw CommonError("ThreadNameController: access to undefined pthread name %s (%s %d)",i->second.c_str(),__FILE__,__LINE__);
    impl->container_pt2str.erase(i);
    impl->container_str2pt.erase(j);
}
void ThreadNameController::remove_LK(const std::string& val)
{
    M_LOCK(impl);
    auto j=impl->container_str2pt.find(val);
    if (j==impl->container_str2pt.end()) throw CommonError("ThreadNameController: access to undefined pthread name %s (%s %d)",val.c_str(),__FILE__,__LINE__);
    auto i=impl->container_pt2str.find(j->second);
    if (i==impl->container_pt2str.end()) throw CommonError("ThreadNameController: access to undefined pthread %X (%s %d)",j->second,__FILE__,__LINE__);
    impl->container_pt2str.erase(i);
    impl->container_str2pt.erase(j);
}
std::string ThreadNameController::getName_LK(const pthread_t& key)
{
    M_LOCK(impl);
    return getName(key);
}
std::string ThreadNameController::getName(const pthread_t& key)
{
    auto i=impl->container_pt2str.find(key);
    if (i==impl->container_pt2str.end()) return std::to_string((int64_t)pthread_self());

    return i->second;
}
std::string ThreadNameController::dump_mutex_inspectors()
{
    M_LOCK(impl);
    std::string out;
    for (auto& i: impl->mutex_inspector_list)
    {

        if (i.second.size())
        {
            if (i.second.begin()->__s.size())
            {
                const pthread_t& pth=i.first;

                std::string s=iUtils->strupper((std::string)i.second.begin()->__s.c_str());
                if (s=="LOOP")
                {
                    out+= "LOOP "+std::to_string((int64_t)pth)+" "+getName(i.first)+"\n";
                    continue;
                }
                if (s=="SLEEP")
                {
                    out+= "SLEEP "+std::to_string((int64_t)pth)+" "+getName(i.first)+"\n";
                    continue;
                }
                if (s=="WAIT")
                {
                    out+= "WAIT "+std::to_string((int64_t)pth)+" "+getName(i.first)+"\n";
                    continue;
                }
                if (s=="USLEEP")
                {
                    out+= "USLEEP "+std::to_string((int64_t)pth)+" "+getName(i.first)+"\n";
                    continue;
                }
                if (s=="SELECT")
                {
                    out+= "SELECT "+std::to_string((int64_t)pth)+" "+getName(i.first)+"\n";
                    continue;
                }
                if (s=="ACCEPT")
                {
                    out+= "ACCEPT "+std::to_string((int64_t)pth)+" "+getName(i.first)+"\n";
                    continue;
                }
            }
        }
        char s[200];
#ifdef _WIN32
        snprintf(s,sizeof(s)-1,"---THREAD REPORT %ld (0x%lX) %s  \n", (long)i.first, (long)i.first,getName(i.first).c_str());
#else
        snprintf(s,sizeof(s)-1,"---THREAD REPORT %ld (0x%lX) %s \n", (long)i.first, (long)i.first,getName(i.first).c_str());
#endif
        out += s;
        time_t mint=0;
        for ( auto& j: i.second)
        {
            if (mint==0) mint=j.t;
            if (j.t<mint) mint=j.t;
        }
        for (auto& j : i.second)
        {
            snprintf(s,sizeof(s)-1,"\t%s: %d, %s %s, dt:%02ld\n", j.f?j.f:"", j.l, j.func, j.__s.c_str(), j.t - mint);
            out+=s;
        }
    }
    return out;
    return	"";
}
std::string ThreadNameController::dump_mutex_inspector(const pthread_t &pt)
{
    M_LOCK(impl);

    std::string out;
    auto i=impl->mutex_inspector_list.find(pt);

    if (i!=impl->mutex_inspector_list.end())
    {
        if (i->second.size())
        {
            if (i->second.begin()->__s.size())
            {
                int64_t pth;
                pth=(int64_t)i->first;

                std::string s=iUtils->strupper((std::string)i->second.begin()->__s.c_str());
                if (s=="LOOP") return "LOOP "+std::to_string(pth)+" "+getName(i->first)+"\n";
                if (s=="SLEEP") return "SLEEP "+std::to_string(pth)+" "+getName(i->first)+"\n";
                if (s=="WAIT") return "WAIT "+std::to_string(pth)+" "+getName(i->first)+"\n";
                if (s=="USLEEP") return "USLEEP "+std::to_string(pth)+" "+getName(i->first)+"\n";
                if (s=="SELECT") return "SELECT "+std::to_string(pth)+" "+getName(i->first)+"\n";
                if (s=="ACCEPT") return "ACCEPT "+std::to_string(pth)+" "+getName(i->first)+"\n";
                if (s=="POLL") return "POLL "+std::to_string(pth)+" "+getName(i->first)+"\n";
                if (s=="MYSQL_REAL_QUERY") return "MYSQL_REAL_QUERY "+std::to_string(pth)+" "+getName(i->first)+"\n";
            }
        }
        time_t tt = time(NULL);
        char s[200];
        snprintf(s,sizeof(s)-1,"---THREAD REPORT %ld (0x%lX) %s\n", (long)i->first, (long)i->first,getName(i->first).c_str());
        out += s;
        for (auto& j: i->second)
        {
            snprintf(s,sizeof(s)-1,"\t%s: %d, %s %s, dt:%02ld (%02ld)\n", j.f?j.f:"", j.l, j.func, j.__s.c_str(), tt - j.t,tt - j.t);
            out+=s;
        }
    }
    return out;
    return	"";
}
void ThreadNameController::push_mi(const pthread_t& pt, mutex_inspector_entry* e)
{
    M_LOCK(impl);
    impl->mutex_inspector_list[pt].push_front(*e);

}
void ThreadNameController::pop_mi(const pthread_t& pt)
{
    M_LOCK(impl);
    if (impl->mutex_inspector_list[pt].size())
    {
        impl->mutex_inspector_list[pt].erase(impl->mutex_inspector_list[pt].begin());
    }
    if (impl->mutex_inspector_list[pt].size() == 0)
    {
        impl->mutex_inspector_list.erase(pt);
    }

}
void ThreadNameController::print_term(int signum)
{
    std::string out;
    out+=dump_mutex_inspectors();
    std::string fn;
    if (signum!=10)
    {
        fn=(std::string)"TERM."+std::to_string(time(NULL));
    }
    else
    {
        fn=(std::string)"USERTERM."+std::to_string(time(NULL));
    }
#ifdef __ANDROID__
    fn="/sdcard/"+fn;
#endif
    if (out.size())
    {
        out="Received on SIGNAL "+std::to_string(signum)+"\n"+out;
#if !defined __MOBILE__

        FILE *f=fopen(fn.c_str(),"wb");
        if (f)
        {
            fprintf(f,"%s",out.c_str());
            fclose(f);
        }
#endif
    }
    logErr2("%s",out.c_str());
}


ThreadNameController::ThreadNameController()
{
    impl=new ThreadNameControllerImpl;
}
ThreadNameController::~ThreadNameController()
{

    delete impl;
}
