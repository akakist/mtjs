#pragma once
#include <string>
#include "REF.h"
#include "db_to_save.h"
#include "THASH_id.h"
#include "THASH_id.h"
#include "bc_contract.h"
#include "bc_address_state.h"
#include "bc_granule.h"
#include "bc_node.h"
#include "bc_nodelist.h"
#include "bc_values.h"
#include "bc_vote.h"
#include "CONTRACT_id.h"
#include "CONTRACT_DATA_id.h"
#include "md/md_BlockAcceptedREQ.h"
#include "md/md_BlockAccepted2REQ.h"
#include "bc_contract_balance.h"
#include "bc_state.h"

struct IDatabase: public Refcountable
{

    virtual std::string getDbName()=0;

    virtual int getGranule(const std::string& k, std::string* v)=0;
    virtual int write_granules_batch(const _db_to_save &v)=0;
    virtual int deleteGranule(const std::string& k) = 0;
    std::atomic<bool> clear_root=false;

    // REF_getter<Cellable> root;


    IDatabase():Refcountable("Idatabse") {}
    void add_sync_out(const std::string &k)
    {
        sync_empty=false;
        M_LOCK(mx);
        sync_out[k.size()].insert(k);
    }
    
    std::function<void()> on_sync_finished;   // вызывается при переходе sync_empty false→true

    void remove_sync_out(const std::string &k)
    {
        // M_LOCK(mx);
        MutexLockerDeferred l(mx);
        l.lock();
        auto &s = sync_out[k.size()];
        s.erase(k);
        if (s.empty()) sync_out.erase(k.size());
        if (sync_out.empty() && !sync_empty.exchange(true)) {
            // был false → стал true: sync завершён
            auto cb = on_sync_finished;   // копия под локом
            // unlock перед вызовом? см. ниже
            l.unlock();
            if (cb) cb();
        }
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
                    if(pathes.size()>1000)
                        break;
                }
                break;
            }
        }
        return pathes;
    }
    void setIp(const std::string& ip)
    {
        M_LOCK(mx);
        sync_ip=ip;
    }
    std::string getIp()
    {
        M_LOCK(mx);
        return sync_ip;
    }
    std::atomic<bool> sync_empty=true;


private:
    std::map<int, std::set<std::string> > sync_out;
    std::string sync_ip;

public:
    Mutex mx;

    
};

REF_getter<Cellable>  getRoot(IDatabase *db, const REF_getter<MsgData::BlockAccepted2REQ>& pb);
REF_getter<MsgData::BlockAccepted2REQ> load_last_block(IDatabase *db);
