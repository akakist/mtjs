

#include <string>
#include <gmp.h>
#include <vector>
#include <string>
#include "bigint.h"
#include "blake2bHasher.h"
#include "blst_cp.h"
#include "ioBuffer.h"
#include <sys/stat.h>
#include "root_contract.h"
#include "QUORUM.h"
#include "md/md_BlockAcceptedREQ.h"

std::vector<data_base *(*)(Cellable *)> db_constructors = {
    +[](Cellable *p) -> data_base *
    { return new bc_contract(p); },

    +[](Cellable *p) -> data_base *
    { return new bc_address_state(p); },

    +[](Cellable *p) -> data_base *
    { return new bc_node(p); },

    +[](Cellable *p) -> data_base *
    { return new bc_nodelist(p); },

    +[](Cellable *p) -> data_base *
    { return new bc_values(p); },

    +[](Cellable *p) -> data_base *
    { return new bc_contract_data(p); },

};


REF_getter<Cellable> getByPathOrCreate(REF_getter<Cellable> cur, const std::vector<std::string> &v, IDatabase *db,Rollback* roll)
{
    for (auto &z : v)
    {
        MutexLockerDeferred l(cur->mx);
        cur = cur->getLeafOrCreate(z, db,l,roll);
    }
    return cur;
}
REF_getter<Cellable> getByPathOrCreate(REF_getter<Cellable> cur, const std::deque<std::string> &v, IDatabase *db,Rollback* roll)
{
    for (auto &z : v)
    {
        MutexLockerDeferred l(cur->mx);
        cur = cur->getLeafOrCreate(z, db,l,roll);
    }
    return cur;
}
REF_getter<Cellable> getByPathNoCreate(REF_getter<Cellable> cur, const std::vector<std::string> &v, IDatabase *db)
{
    // M_LOCK(cur->lock);
    for (auto &z : v)
    {
        MutexLockerDeferred l(cur->mx);
        cur = cur->getLeafNoCreate(z, db,l);
        if (!cur.valid())
            return NULL;
    }
    return cur;
}
std::vector<std::string> root_data::getPath(const std::string& name) 
{
    std::vector<std::string> out;
    auto h = blake2b_hash(name);
    out.reserve(5);
    out.emplace_back(h.container, 0, 1);
    out.emplace_back(h.container, 1, 1);
    out.emplace_back(h.container, 2, 1);
    out.emplace_back(h.container, 3, 1);
    out.emplace_back(h.container, 4, 28);
    return out;
}

std::vector<std::string> root_data::getContractPath(const CONTRACT_id &name)
{
    return getPath("CONTRACT_"+name.container);
}
std::vector<std::string> root_data::getContractDataPath(const CONTRACT_DATA_id &name)
{
    std::string key="CONTRACT_DATA_"+name.container;    
    return getPath(key);

}

std::vector<std::string> root_data::getNodePath(const NODE_id &name)
{
    return getPath("NODE_"+name.container);
}

REF_getter<bc_contract> root_data::getContract(const CONTRACT_id &name,IDatabase* db)
{
    MUTEX_INSPECTOR;
    auto cc = getByPathNoCreate(this, getContractPath(name), db);
    if (!cc.valid())
        return NULL;
    if (!cc->data.valid())
        throw CommonError("if(!cc->data.valid())");
    return dynamic_cast<bc_contract *>(cc->data.get());
}
REF_getter<bc_contract_data> root_data::getContractData(const CONTRACT_DATA_id &name,IDatabase* db)
{
    MUTEX_INSPECTOR;
    auto cc = getByPathNoCreate(this, getContractDataPath(name), db);
    if (!cc.valid())
        return NULL;
    if (!cc->data.valid())
        throw CommonError("if(!cc->data.valid())");
    return dynamic_cast<bc_contract_data *>(cc->data.get());
}
REF_getter<bc_nodelist> root_data::getNodeListOrCreate(Rollback* roll,IDatabase* db)
{
    MUTEX_INSPECTOR;
    auto cc = getByPathOrCreate(this, getPath("NODE LIST"), db,roll);
    if (!cc.valid())
        return NULL;
    if (cc->data.valid())
    {
        return dynamic_cast<bc_nodelist *>(cc->data.get());
    }

    REF_getter<bc_nodelist> u = new bc_nodelist(cc.get());
    cc->data = u.get();
    cc->payload_ctor_idx = hsh::bc_nodelist;
    return u;
}
REF_getter<bc_nodelist> root_data::getNodeListNoCreate(IDatabase* db)
{
    MUTEX_INSPECTOR;
    auto cc = getByPathNoCreate(this, getPath("NODE LIST"), db);
    if (!cc.valid())
        return NULL;
    if (cc->data.valid())
    {
        return dynamic_cast<bc_nodelist *>(cc->data.get());
    }

    return NULL;
}

REF_getter<bc_contract> root_data::addContract(const CONTRACT_id &name, Rollback* roll,IDatabase* db)
{
    MUTEX_INSPECTOR;
    auto cc = getByPathOrCreate(this, getContractPath(name), db,roll);
    if (!cc.valid())
        throw CommonError("if(!cc.valid())");
    if (cc->data.valid())
        throw CommonError("if(cc->data.valid())");
    REF_getter<bc_contract> bc = new bc_contract(cc.get());
    cc->data = bc.get();
    cc->payload_ctor_idx = hsh::bc_contract;
    cc->data->setDirty(roll);
    return bc;
}
REF_getter<bc_contract_data> root_data::addContractData(const CONTRACT_DATA_id &name, Rollback* roll,IDatabase* db)
{
    MUTEX_INSPECTOR;
    auto cc = getByPathOrCreate(this, getContractDataPath(name), db,roll);
    if (!cc.valid())
        throw CommonError("if(!cc.valid())");
    if (cc->data.valid())
        throw CommonError("if(cc->data.valid())");
    REF_getter<bc_contract_data> bc = new bc_contract_data(cc.get());
    cc->data = bc.get();
    cc->payload_ctor_idx = hsh::bc_contract_data;
    cc->data->setDirty(roll);
    return bc;
}

REF_getter<bc_values> root_data::getValues(Rollback* roll,IDatabase* db)
{
    MUTEX_INSPECTOR;
    auto r = this;
    MutexLockerDeferred lk(r->mx);
    auto l = getByPathOrCreate(this, getPath("VALUES"), db, roll);
    if (!l.valid())
        throw CommonError("if(!l.valid())");
    if (l->data.valid())
    {
        return dynamic_cast<bc_values *>(l->data.get());
    }

    REF_getter<bc_values> v = new bc_values(l.get());
    lk.lock();
    l->data = v.get();
    l->payload_ctor_idx = hsh::bc_values;
    lk.unlock();
    return v;
}
REF_getter<bc_values> root_data::checkValues(IDatabase* db)
{
    MUTEX_INSPECTOR;
    auto r = this;
    MutexLockerDeferred lk(r->mx);
    auto l = getByPathNoCreate(this, getPath("VALUES"), db);
    if (!l.valid())
        return NULL;
    lk.lock();
    auto data=l->data;
    lk.unlock();
    if (data.valid())
    {
        return dynamic_cast<bc_values *>(data.get());
    }
    return NULL;
}

std::vector<std::string> root_data::getUserPath(const ADDRESS_id &addr)
{
    MUTEX_INSPECTORS("getUserPath");
    return getPath("USER_"+addr.addr);
}
std::vector<std::string> root_data::getAddressStatePath(const ADDRESS_id &addr)
{
    MUTEX_INSPECTOR;
    return getPath("USER_STATE_"+addr.addr);
}

REF_getter<bc_address_state> root_data::getAddressState(const ADDRESS_id &addr, Rollback* roll,IDatabase* db)
{
    MUTEX_INSPECTOR;
    auto cc = getByPathOrCreate(this, getAddressStatePath(addr), db,roll);
    if (!cc.valid())
        return NULL;
    if (cc->data.valid())
    {
        return dynamic_cast<bc_address_state *>(cc->data.get());
    }

    REF_getter<bc_address_state> u = new bc_address_state(cc.get());
    cc->data = u.get();
    cc->payload_ctor_idx = hsh::bc_address_state;
    return u;
}
REF_getter<bc_address_state> root_data::checkUserState(const ADDRESS_id &addr,IDatabase* db)
{
    MUTEX_INSPECTOR;
    auto cc = getByPathNoCreate(this, getAddressStatePath(addr), db);
    if (!cc.valid())
        return NULL;

    return dynamic_cast<bc_address_state *>(cc->data.get());
}

std::vector<REF_getter<bc_node>> root_data::getAllNodes(IDatabase* db)
{
    MUTEX_INSPECTOR;
    auto nl=getNodeListNoCreate(db);
    if(!nl.valid())
        throw CommonError("if(!nl.valid())");

    std::vector<REF_getter<bc_node>> vv;

    for(auto& z: nl->list)
    {
        vv.push_back(getNode(z,db));
    }
    return vv;

}
REF_getter<bc_node> root_data::addNode(const NODE_id &name, Rollback* roll,IDatabase* db)
{
    MUTEX_INSPECTOR;

    auto nl=getNodeListOrCreate(roll,db);
    if(!nl.valid())
        throw CommonError("if(!nl.valid())");
    if(nl->list.count(name))
        throw CommonError("if(nl->list.count(name))");
    nl->list.insert(name);
    nl->setDirty(roll);

    auto cc = getByPathOrCreate(this, getNodePath(name), db,roll);

    if (cc->data.valid())
        throw CommonError("if(cc->data.valid())");

        REF_getter<bc_node> n = new bc_node(cc.get());
    cc->data = n.get();
    cc->payload_ctor_idx = hsh::bc_node;
    return n;
}

REF_getter<bc_node> root_data::getNode(const NODE_id &name, IDatabase* db)
{
    MUTEX_INSPECTOR;

    auto cc = getByPathNoCreate(this, getNodePath(name), db);
    if (!cc.valid())
        return NULL;
    if (cc->data.valid())
        return dynamic_cast<bc_node *>(cc->data.get());
    else
        throw CommonError("if(cc->data.valid())");
}

std::pair<REF_getter<root_data>,REF_getter<MsgData::BlockAcceptedREQ>>  getRoot(IDatabase *db)
{
    MUTEX_INSPECTOR;

    REF_getter<root_data> r = new root_data();
    REF_getter<MsgData::BlockAcceptedREQ> last_block;
    std::string root_cell;
    {
    MUTEX_INSPECTOR;
        int err = db->getGranule("#root#", &root_cell);
        if (!err)
        {
    MUTEX_INSPECTOR;
            {
                MUTEX_INSPECTOR;
                inBuffer in(root_cell);
                M_LOCK(r->mx);
                r->unpack_mx(in);
            }
        }
        if(!err)
        {
    MUTEX_INSPECTOR;
            std::string lb;
            int err = db->getGranule("#last_block#", &lb);
            if(!err && lb.size())
            {
    MUTEX_INSPECTOR;
                last_block=new MsgData::BlockAcceptedREQ;
                // last_block->unpack2
                inBuffer in(lb);
                last_block->unpack2(in);

            }
            if(!err && lb.empty())
            {
    MUTEX_INSPECTOR;
            }
        }

    }

    return {r,last_block};
}
