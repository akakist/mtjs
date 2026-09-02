#pragma once
#include <string>
#include "REF.h"
#include "db_to_save.h"
#include "THASH_id.h"
#include "THASH_id.h"
struct IDatabase: public Refcountable
{

    virtual std::string getDbName()=0;

    virtual int getGranule(const std::string& k, std::string* v)=0;
    virtual int write_granules_batch(const _db_to_save &v)=0;
    // virtual bool getBlock(const THASH_id &h, std::string& block) = 0;
    // virtual bool writeBlock(uint64_t epoch, uint64_t block_timestamp,  const std::string& prev_root_hash, const std::string& data)=0;
    std::atomic<bool> clear_root=false;
    


    IDatabase():Refcountable("Idatabse") {}
    void add_sync_out(const std::string &k)
    {
        sync_empty=false;
        M_LOCK(mx);
        sync_out[k.size()].insert(k);
    }
    void remove_sync_out(const std::string &k)
    {
        M_LOCK(mx);
        auto &s=sync_out[k.size()];
        s.erase(k);
        if(s.empty())
            sync_out.erase(k.size());
        if(sync_out.empty())
            sync_empty=true;
    }
    std::vector<std::string> getPathes()
    {
        std::vector<std::string> pathes;
        M_LOCK(mx);
        for(auto &z: sync_out)
        {
            if(z.second.size())
            {
                for(auto& x: z.second)
                {
                    pathes.push_back(x);
                    // z.second.erase(x);
                    if(pathes.size()>1000)
                        break;
                }
                break;
            }
        }
        return pathes;
    }

    std::atomic<bool> sync_empty=true;

    private:
    Mutex mx;
    std::map<int, std::set<std::string> > sync_out;
    
};