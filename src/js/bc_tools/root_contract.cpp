

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

bool root_data::verify_lider_certificate(const REF_getter<MsgData::LeaderCertificate> &lc)
{
    /// проверка сертификата лидера
    {
        MUTEX_INSPECTOR;
        std::vector<blst_cpp::PublicKey> agg_pk;
        BigInt stake;
        for (auto &z : lc->nodes)
        {
            auto n = this->getNode(z);
            if (!n.valid())
            {
                logErr2("            if (!n.valid()) %s",z.container.c_str());
                return false;

            }
            agg_pk.push_back(n->get_bls_pk());
            stake += n->get_full_stake();
        }
        auto nn=getAllNodes();
        BigInt ts = 0;
        for(auto &z: nn)
        {
            ts+=z->get_full_stake();
        }
        if (stake.toDouble() < ts.toDouble() * QUORUM)
        {
            logErr2("verify lc quorum failed");
            return false;
        }
        // throw CommonError("if(stake.toDouble() < root->getValues(NULL)->total_staked.toDouble() * QUORUM)");
        if (!lc->agg_sig.verify(agg_pk, blake2b_hash(lc->heart_beat->getBuffer()).container))
        {
            logErr2("verify lc - sign invalid");
            ;
            return false;
        }
    }

    return true;
}

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

std::vector<std::string> root_data::getContractPath(const std::string &name)
{
    std::vector<std::string> p;
    p.push_back("c");
    appendRelativeInternalPath(p, name, 3);
    // p.push_back(name);
    return p;
}

std::vector<std::string> root_data::getNodePath(const std::string &name)
{
    std::vector<std::string> p;
    p.push_back("n");
    appendRelativeInternalPath(p, name, 2);
    // p.push_back(name);
    return p;
}

REF_getter<bc_contract> root_data::getContract(const std::string &name)
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

REF_getter<bc_contract> root_data::addContract(const std::string &name, const REF_getter<fee_calcer> &bca)
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
    cc->setDirty();
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

std::vector<std::string> root_data::getUserPath(const std::string &pk_bin)
{
    MUTEX_INSPECTORS("getUserPath");
    if (pk_bin.size() != 32)
        throw CommonError("    if(pk_bin.size()!=32) %d %s", pk_bin.size(), _DMI().c_str());
    std::vector<std::string> p;
    p.push_back("u");
    std::string pk_hex = base16::encode(pk_bin);
    appendRelativeInternalPath(p, pk_hex, 5);
    // p.push_back(pk_hex);
    return p;
}
std::vector<std::string> root_data::getUserStatePath(const std::string &pk_bin)
{
    MUTEX_INSPECTORS("getUserStatePath");
    if (pk_bin.size() != 32)
        throw CommonError("    if(pk_bin.size()!=32) %d %s", pk_bin.size(), _DMI().c_str());
    std::vector<std::string> p;
    p.reserve(10);
    p.push_back("s");
    std::string pk_hex = base16::encode(pk_bin);
    appendRelativeInternalPath(p, pk_hex, 5);
    // p.push_back(pk_hex);
    return p;
}

REF_getter<bc_user> root_data::getUser(const std::string &pk)
{
    MUTEX_INSPECTOR;
    auto v = getUserPath(pk);
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
REF_getter<bc_user_state> root_data::getUserState(const std::string &pk)
{
    MUTEX_INSPECTOR;
    auto v = getUserStatePath(pk);
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
REF_getter<bc_user_state> root_data::checkUserState(const std::string &pk)
{
    MUTEX_INSPECTOR;
    auto v = getUserStatePath(pk);
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
#ifdef KALL
    for (auto &z : c->children_hashes_mx)
    {
        auto key = z.first;
        auto it = c->children_ptrs_mx.find(key);
        if (it != c->children_ptrs_mx.end())
        {
            getChildrenRecursive(it->second.get(), res, db);
        }
        else
        {
            auto cc = c->getLeafNoCreate(key, db);
            if (!cc.valid())
            {
                throw CommonError("if(!cc.valid())");
            }
            getChildrenRecursive(cc.get(), res, db);
        }
    }
#endif
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
REF_getter<bc_node> root_data::addNode(const NODE_id &name, const REF_getter<fee_calcer> &bc)
{
    MUTEX_INSPECTOR;

    std::vector<std::string> v = getNodePath(name.container);
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
    cc->setDirty();
    return n;
}

REF_getter<bc_node> root_data::getNode(const NODE_id &name)
{
    MUTEX_INSPECTOR;
    std::vector<std::string> v = getNodePath(name.container);
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
