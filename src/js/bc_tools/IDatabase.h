#pragma once
#include <string>
#include "REF.h"
#include "db_to_save.h"
#include "THASH_id.h"
#include "THASH_id.h"
#include "bc_contract.h"
#include "bc_address_state.h"
#include "bc_contract_data.h"
#include "bc_node.h"
#include "bc_nodelist.h"
#include "bc_values.h"
#include "CONTRACT_id.h"
#include "CONTRACT_DATA_id.h"
#include "md/md_BlockAcceptedREQ.h"

// #include "root_contract.h"
// class root_data;
struct IDatabase: public Refcountable
{

    virtual std::string getDbName()=0;

    virtual int getGranule(const std::string& k, std::string* v)=0;
    virtual int write_granules_batch(const _db_to_save &v)=0;
    // virtual bool getBlock(const THASH_id &h, std::string& block) = 0;
    // virtual bool writeBlock(uint64_t epoch, uint64_t block_timestamp,  const std::string& prev_root_hash, const std::string& data)=0;
    std::atomic<bool> clear_root=false;

    REF_getter<Cellable> root;


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
    Mutex mx;
    std::map<int, std::set<std::string> > sync_out;
    std::string sync_ip;
    // REF_getter<root_data> _root;

    public:
    static std::vector<std::string> getPath(const std::string& name);


    static std::vector<std::string> getContractPath(const CONTRACT_id &name);
    static std::vector<std::string> getContractDataPath(const CONTRACT_DATA_id &name);
    static std::vector<std::string> getNodePath(const NODE_id &name);


    static std::vector<std::string> getUserPath(const ADDRESS_id &addr);
    static std::vector<std::string> getAddressStatePath(const ADDRESS_id &addr);

    REF_getter<bc_contract> getContract(const CONTRACT_id &name);
    REF_getter<bc_contract> addContract(const CONTRACT_id &name, Rollback*);

    REF_getter<bc_contract_data> getContractData(const CONTRACT_DATA_id &name);
    REF_getter<bc_contract_data> addContractData(const CONTRACT_DATA_id &name,  Rollback*);

    REF_getter<bc_values> getValuesOrCreate(Rollback*);
    REF_getter<bc_values> getValuesNoCreate();
    REF_getter<bc_values> checkValues();


    REF_getter<bc_address_state> getAddressState(const ADDRESS_id &pk,Rollback*);
    REF_getter<bc_address_state>   checkUserState(const ADDRESS_id &pk);


    // std::vector<NODE_id> getNodesNames();
    std::vector<REF_getter<bc_node>> getAllNodes();


    REF_getter<bc_node> getNode(const NODE_id &name);
    REF_getter<bc_node> addNode(const NODE_id &name, Rollback*);
    REF_getter<bc_nodelist> getNodeListOrCreate(Rollback* roll);
    REF_getter<bc_nodelist> getNodeListNoCreate();

    REF_getter<Cellable> getLeafNoCreate(const REF_getter<Cellable>&cur, const std::string& id, MutexLockerDeferred &l);
    REF_getter<Cellable> getLeafOrCreate(const REF_getter<Cellable>&cur, const std::string &id, MutexLockerDeferred &l, Rollback *roll);


    REF_getter<Cellable> getByPathOrCreate(const REF_getter<Cellable>&cur, const std::vector<std::string>& v, Rollback* roll);
    REF_getter<Cellable> getByPathOrCreate(const REF_getter<Cellable>&cur, const std::deque<std::string> &v, Rollback*);

    REF_getter<Cellable> getByPathNoCreate(const REF_getter<Cellable>&cur, const std::vector<std::string>& v);

  
};

REF_getter<Cellable>  getRoot(IDatabase *db, const REF_getter<MsgData::BlockAcceptedREQ>& pb);
REF_getter<MsgData::BlockAcceptedREQ> load_last_block(IDatabase *db);
