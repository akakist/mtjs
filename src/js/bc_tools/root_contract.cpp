

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
#include "fee_calcer.h"

std::vector<data_base *(*)(Cellable *)> db_constructors = {
    +[](Cellable *p) -> data_base *
    { return new bc_contract(p); },
    +[](Cellable *p) -> data_base *
    { return new bc_user(p); },
    +[](Cellable *p) -> data_base *
    { return new bc_user_state(p); },
    +[](Cellable *p) -> data_base *
    { return new bc_node(p); },
    +[](Cellable *p) -> data_base *
    { return new bc_values(p); },
    +[](Cellable *p) -> data_base *
    { return new bc_epoch(p); }
};


REF_getter<Cellable> getByPathOrCreate(REF_getter<Cellable> cur, const std::vector<std::string> &v, IDatabase *db)
{
    // M_LOCK(cur->lock);
    for (auto &z : v)
    {
        MutexLockerDeferred l(cur->mx);
        cur = cur->getLeafOrCreate(z, db,l);
    }
    return cur;
}
REF_getter<Cellable> getByPathOrCreate(REF_getter<Cellable> cur, const std::deque<std::string> &v, IDatabase *db)
{
    // M_LOCK(cur->lock);
    for (auto &z : v)
    {
        MutexLockerDeferred l(cur->mx);
        cur = cur->getLeafOrCreate(z, db,l);
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
void root_data::getDiff(cdiff& out, const EPOCH_id &epoch)
{
    M_LOCK(state_mutex);
    _getDiff(out,epoch,db.get());
}

void root_data::apply_diff(const cdiff& out)
{
    M_LOCK(state_mutex);
    for(auto &e: out.container)
    {
        auto &epoch=e.first;
        for(auto &z:e.second)
        {
            auto& path=z.first;
            auto& buf=z.second.first;
            int ctor=z.second.second;
            REF_getter<Cellable> cell=getByPathOrCreate(this,path,db.get());
            if(cell->payload_ctor_idx && cell->payload_ctor_idx!=ctor)
            {
                throw CommonError("if(cell->payload_ctor_idx && cell->payload_ctor_idx!=ctor)");
            }
            if(cell->payload_ctor_idx!=ctor)
                cell->payload_ctor_idx=ctor;
            data=db_constructors[cell->payload_ctor_idx](cell.get());
            inBuffer in(buf);
            data->unpack(in);
            if(data->last_update_epoch!=epoch)
                throw CommonError("if(data->last_update_epoch!=epoch)");
            data->setDirty(data->last_update_epoch);
        }
    }
}
// void getDiff(std::map<std::string,std::string>& out)
// {

// }

std::vector<std::string> root_data::getContractPath(const CONTRACT_id &name)
{
    std::vector<std::string> p;
    p.push_back("c");
    auto h=ghash(name.container.c_str());
    char buf[2];
    buf[0] = "0123456789abcdef"[h % 16];
    buf[1] = "0123456789abcdef"[(h >> 8) % 16];
    p.push_back({buf,1});
    p.push_back({buf+1,1});
    p.push_back(name.container);
    return p;
}

std::vector<std::string> root_data::getNodePath(const NODE_id &name)
{
    std::vector<std::string> p;
    p.push_back("n");
    auto h=ghash(name.container.c_str());
    char buf[2];
    buf[0] = "0123456789abcdef"[h % 16];
    buf[1] = "0123456789abcdef"[(h >> 8) % 16];
    p.push_back({buf,1});
    p.push_back({buf+1,1});
    p.push_back(name.container);

    return p;
}

REF_getter<bc_contract> root_data::getContract(const CONTRACT_id &name)
{
    MUTEX_INSPECTOR;
    auto v = getContractPath(name);
    auto cc = getByPathNoCreate(this, v, db.get());
    if (!cc.valid())
        return NULL;
    if (!cc->data.valid())
        throw CommonError("if(!cc->data.valid())");
    return dynamic_cast<bc_contract *>(cc->data.get());
}

REF_getter<bc_contract> root_data::addContract(const CONTRACT_id &name, const REF_getter<fee_calcer> &bca, const EPOCH_id& epoch)
{
    MUTEX_INSPECTOR;
    auto v = getContractPath(name);
    auto cc = getByPathOrCreate(this, v, db.get());
    if (!cc.valid())
        throw CommonError("if(!cc.valid())");
    if (cc->data.valid())
        throw CommonError("if(cc->data.valid())");
    REF_getter<bc_contract> bc = new bc_contract(cc.get());
    cc->data = bc.get();
    cc->payload_ctor_idx = hsh::bc_contract;
    cc->data->setDirty(epoch);
    return bc;
}
REF_getter<bc_epoch> root_data::getEpoch()
{
    MUTEX_INSPECTOR;
    auto r = this;
    MutexLockerDeferred lk(r->mx);
    auto l = r->getLeafOrCreate("ep", db.get(),lk);
    if (!l.valid())
        throw CommonError("if(!l.valid())");
    if (l->data.valid())
    {
        return dynamic_cast<bc_epoch *>(l->data.get());
    }

    REF_getter<bc_epoch> v = new bc_epoch(l.get());
    lk.lock();
    l->data = v.get();
    l->payload_ctor_idx = hsh::bc_epoch;
    lk.unlock();
    return v;
}

REF_getter<bc_values> root_data::getValues()
{
    MUTEX_INSPECTOR;
    auto r = this;
    MutexLockerDeferred lk(r->mx);
    auto l = r->getLeafOrCreate("v", db.get(),lk);
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
REF_getter<bc_values> root_data::checkValues()
{
    MUTEX_INSPECTOR;
    auto r = this;
    MutexLockerDeferred lk(r->mx);
    auto l = r->getLeafNoCreate("v", db.get(),lk);
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
    if (addr.addr.size() != 32)
        throw CommonError("    if(pk_bin.size()!=32) %d %s", addr.addr.size(), _DMI().c_str());
    std::vector<std::string> p;
    p.push_back("u");
    std::string addr_hex = base16::encode(addr.addr);
    appendRelativeInternalPath(p, addr_hex, 5);
    // p.push_back(pk_hex);
    return p;
}
std::vector<std::string> root_data::getUserStatePath(const ADDRESS_id &addr)
{
    MUTEX_INSPECTORS("getUserStatePath");
    if (addr.addr.size() != 32)
        throw CommonError("    if(pk_bin.size()!=32) %d %s", addr.addr.size(), _DMI().c_str());
    std::vector<std::string> p;
    p.reserve(10);
    p.push_back("s");
    std::string addr_hex = base16::encode(addr.addr);
    appendRelativeInternalPath(p, addr_hex, 5);
    // p.push_back(pk_hex);
    return p;
}

REF_getter<bc_user> root_data::getUser(const ADDRESS_id &addr)
{
    MUTEX_INSPECTOR;
    auto v = getUserPath(addr);
    auto cc = getByPathOrCreate(this, v, db.get());
    if (!cc.valid())
        return NULL;
    if (cc->data.valid())
    {
        return dynamic_cast<bc_user *>(cc->data.get());
    }
    // if(cc->payload_.size())
    // throw CommonError("if(cc->payload_.size())");
    // else throw CommonError("if(cc->data.valid())");
    REF_getter<bc_user> u = new bc_user(cc.get());
    cc->data = u.get();
    cc->payload_ctor_idx = hsh::bc_user;
    return u;
}
REF_getter<bc_user_state> root_data::getUserState(const ADDRESS_id &addr)
{
    MUTEX_INSPECTOR;
    auto v = getUserStatePath(addr);
    auto cc = getByPathOrCreate(this, v, db.get());
    if (!cc.valid())
        return NULL;
    if (cc->data.valid())
    {
        return dynamic_cast<bc_user_state *>(cc->data.get());
    }
    // else throw CommonError("if(cc->data.valid())");
    // if(cc->payload_.size())
    //     throw CommonError("if(cc->payload_.size())");

    REF_getter<bc_user_state> u = new bc_user_state(cc.get());
    cc->data = u.get();
    cc->payload_ctor_idx = hsh::bc_user_state;
    return u;
}
REF_getter<bc_user_state> root_data::checkUserState(const ADDRESS_id &addr)
{
    MUTEX_INSPECTOR;
    auto v = getUserStatePath(addr);
    auto cc = getByPathNoCreate(this, v, db.get());
    if (!cc.valid())
        return NULL;

    return dynamic_cast<bc_user_state *>(cc->data.get());
}

void getChildrenRecursive(Cellable *c, std::vector<REF_getter<data_base>> &res, IDatabase *db)
{
    MUTEX_INSPECTOR;
    MutexLockerDeferred lk(c->mx);

    lk.lock();
    if (c->data.valid())
    {
        res.push_back(c->data);
    }
    std::vector<std::string> v;

    v.reserve(c->children_hashes_mx.size());

    for (auto &z : c->children_hashes_mx)
    {
        v.push_back(z.first);
    }
    for (auto &key : v)
    {
        MUTEX_INSPECTOR;
        auto it = c->children_ptrs_mx.find(key);
        if (it != c->children_ptrs_mx.end())
        {
            MUTEX_INSPECTOR;
            lk.unlock();
            getChildrenRecursive(it->second.get(), res, db);
            lk.lock();
        }
        else
        {
            MUTEX_INSPECTOR;
            lk.unlock();

            auto cc = c->getLeafNoCreate(key, db,lk);
            if (!cc.valid())
            {
                throw CommonError("if(!cc.valid())");
            }
            getChildrenRecursive(cc.get(), res, db);
            lk.lock();
        }
    }
}
std::vector<REF_getter<bc_node>> root_data::getAllNodes()
{
    MUTEX_INSPECTOR;

    std::vector<REF_getter<data_base>> v;

    MutexLockerDeferred lk(mx);

    auto n = getLeafNoCreate("n", db.get(),lk);

    getChildrenRecursive(n.get(), v, db.get());

    std::vector<REF_getter<bc_node>> vv;
    for (auto &z : v)
    {
        if (z->type == hsh::bc_node)
        {
            auto *p = dynamic_cast<bc_node *>(z.get());
            if (p)
                vv.push_back(p);
            else
                throw CommonError("if(p)");
        }
    }
    return vv;
    // for (auto &z : n->children_hashes)
    // {
    //     NODE_id n;
    //     n.container = z.first;
    //     v.push_back(n);
    // }
    // return v;
}
REF_getter<bc_node> root_data::addNode(const NODE_id &name, const REF_getter<fee_calcer> &bc, const EPOCH_id& epoch)
{
    MUTEX_INSPECTOR;

    std::vector<std::string> v = getNodePath(name);
    // v.push_back("n");
    // v.push_back(name.container);
    auto cc = getByPathOrCreate(this, v, db.get());

    if (cc->data.valid())
        throw CommonError("if(cc->data.valid())");
    // if(cc->payload_.size())
    //     throw CommonError("if(cc->payload.size())");
    REF_getter<bc_node> n = new bc_node(cc.get());
    cc->data = n.get();
    cc->payload_ctor_idx = hsh::bc_node;
    cc->data->setDirty(epoch);
    return n;
}

REF_getter<bc_node> root_data::getNode(const NODE_id &name)
{
    MUTEX_INSPECTOR;
    std::vector<std::string> v = getNodePath(name);
    // v.push_back("n");
    // v.push_back(name.container);

    auto cc = getByPathNoCreate(this, v, db.get());
    if (!cc.valid())
        return NULL;
    if (cc->data.valid())
        return dynamic_cast<bc_node *>(cc->data.get());
    else
        throw CommonError("if(cc->data.valid())");
}

REF_getter<root_data> getRoot(IDatabase *db)
{
    MUTEX_INSPECTOR;

    REF_getter<root_data> r = new root_data(db);
    THASH_id root_hash;
    std::string root_cell;
    int err1 = db->get_cell("#root_hash#", &root_hash.container);
    if (!err1)
    {
        int err = db->get_cell("#root#", &root_cell);
        if (!err)
        {
            if (blake2b_hash(root_cell) != root_hash)
            {
                logErr2("if(blake2b_hash(root_cell)!=root_hash)");
            }
            else
            {
                MUTEX_INSPECTOR;
                inBuffer in(root_cell);
                r->unpack(in);
            }
        }
    }
    else
        logErr2("cannot read #root_hash#");

    return r;
}
#include <nlohmann/json.hpp>
std::string bc_contract::dump()
{
    nlohmann::json j;
    j["create_time"]=create_time;
    j["ttl"]=ttl;
    j["name"]=name_;
    j["owner"]=base16::encode(owner.addr);
    j["src"]=src;
    // std::ostringstream o;
    // o<<"Contract: "  << name_ << std::endl;
    // o<< "Owner: " << base16::encode(owner) << std::endl;
    // o<< "Src: " << src << std::endl;

    return j.dump(2);
}
std::string bc_user_state::dump()
{
    M_LOCK(parent->mx);
    nlohmann::json j;
    // j["balance"]=balance.toString();
    j["nonce"]=nonce;
    return j.dump(2);
}

std::string bc_node::dump()
{
    MUTEX_INSPECTOR;
    M_LOCK (parent->mx);
    nlohmann::json j;
    j["owner"]=base16::encode(owner_address.addr);
    j["ed_pk"]=base16::encode(ed_pk);
    j["bls_pk"]=base16::encode(bls_pk.serialize());
    j["name"]=name_.container;
    j["ip"]=ip;
    j["missed_rounds"]=missed_rounds;
    BigInt total_stake=0;
    for(auto &z: stakes)
    {
        nlohmann::json js;
        js["address"]=base16::encode(z.first.addr);
        js["amount"]=z.second.toString();
        // js.push_back(base16::encode(z.first));
        // js.push_back(z.second.toString());
        j["stakes"].push_back(js);
        total_stake+=z.second;
    }
    j["total_stake"]=total_stake.toString();
    logErr2("j %s",j.dump(2).c_str());
    return j.dump(2);
}

std::string bc_values::dump()
{
    /*
        std::map<std::string,BigInt> fees;
    std::set<std::string> emitters_bin;
*/
    nlohmann::json j;
    for(auto &z: emitters_bin)
    {
        ADDRESS_id a;
        a.addr=blake2b_hash(z.addr).container;
        j["emitters"].push_back(base16::encode(a.addr));
    }
    for(auto& z: fees)
    {
        nlohmann::json jj;
        jj["type"]=z.first;
        jj["value"]=z.second.toString();
        j["fees"].push_back(jj);
    }
    return j.dump(2);
}
std::string bc_epoch::dump() 
{
    nlohmann::json j;
    j["epoch"]=epoch.container;
    if(prev_lc.size())
    {
        inBuffer in(prev_lc);
        REF_getter<MsgData::LeaderCertificate> lc=new MsgData::LeaderCertificate();
        lc->unpack2(in);
        auto &jp=j["prev_lc"];
        for(auto& z: lc->nodes)
        {
            jp["signers"].push_back(z.container);
        }
        jp["aggsig"]=base16::encode(lc->agg_sig.serialize());
        auto &hb=jp["hb"];
        hb["block_timestamp"]=lc->heart_beat->block_timestamp;
        hb["node_leader"]=lc->heart_beat->node_leader.container;
        hb["epoch"]=lc->heart_beat->new_epoch.container;
        hb["prev_root_hash"]=base16::encode(lc->heart_beat->prev_root_hash_1.container);
        return j.dump(2);

        // j["prev_lc"]["epoch"]=lc->
    }
// BigInt epoch;
// std::string prev_lc;
    return "Values";
}
