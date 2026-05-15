

#include <string>
#include <gmp.h>
#include <iostream>
#include <vector>
#include <string>
#include "blst_cp.h"
#include "ioBuffer.h"
#include "ghash.h"
#include <sys/stat.h>
#include "root_contract.h"
#include "msg.h"
#include "QUORUM.h"

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
    { return new bc_epoch(p); }};

bool root_data::verify_lider_certificate(const REF_getter<MsgEvt::LeaderCertificate> &lc)
{
    /// проверка сертификата лидера
    {
        MUTEX_INSPECTOR;
        std::vector<blst_cpp::PublicKey> agg_pk;
        BigInt stake;
        for (auto &z : lc->nodes)
        {
            auto n = this->getNode(z);
            agg_pk.push_back(n->bls_pk);
            stake += n->total_stake;
        }
        if (stake.toDouble() < this->getValues()->total_staked.toDouble() * QUORUM)
            throw CommonError("if(stake.toDouble() < root->getValues(NULL)->total_staked.toDouble() * QUORUM)");
        if (!lc->agg_sig.verify(agg_pk, blake2b_hash(lc->heart_beat->getBuffer()).container))
        {
            return false;
        }
    }

    return true;
}

REF_getter<Cellable> getByPathOrCreate(REF_getter<Cellable> cur, const std::vector<std::string> &v, IDatabase *db)
{
    for (auto &z : v)
    {
        cur = cur->getLeafOrCreate(z, db);
    }
    return cur;
}
REF_getter<Cellable> getByPathNoCreate(REF_getter<Cellable> cur, const std::vector<std::string> &v, IDatabase *db)
{
    for (auto &z : v)
    {
        cur = cur->getLeafNoCreate(z, db);
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
    p.push_back(name);
    return p;
}

std::vector<std::string> root_data::getNodePath(const std::string &name)
{
    std::vector<std::string> p;
    p.push_back("n");
    p.push_back(name);
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
    cc->setDirty(bca);
    return bc;
}
REF_getter<bc_epoch> root_data::getEpoch()
{
    MUTEX_INSPECTOR;
    auto r = this;
    auto l = r->getLeafOrCreate("ep", db.get());
    if (!l.valid())
        throw CommonError("if(!l.valid())");
    if (l->data.valid())
    {
        return dynamic_cast<bc_epoch *>(l->data.get());
    }

    REF_getter<bc_epoch> v = new bc_epoch(l.get());
    l->data = v.get();
    l->payload_ctor_idx = hsh::bc_epoch;
    return v;
}

REF_getter<bc_values> root_data::getValues()
{
    MUTEX_INSPECTOR;
    auto r = this;
    auto l = r->getLeafOrCreate("v", db.get());
    if (!l.valid())
        throw CommonError("if(!l.valid())");
    if (l->data.valid())
    {
        return dynamic_cast<bc_values *>(l->data.get());
    }

    REF_getter<bc_values> v = new bc_values(l.get());
    l->data = v.get();
    l->payload_ctor_idx = hsh::bc_values;
    return v;
}
REF_getter<bc_values> root_data::checkValues()
{
    MUTEX_INSPECTOR;
    auto r = this;
    auto l = r->getLeafNoCreate("v", db.get());
    if (!l.valid())
        return NULL;
    if (l->data.valid())
    {
        return dynamic_cast<bc_values *>(l->data.get());
    }
    return NULL;
}

std::vector<std::string> root_data::getUserPath(const std::string &pk)
{
    std::vector<std::string> p;
    p.push_back("u");
    appendRelativeInternalPath(p, pk, 3);
    p.push_back(pk);
    return p;
}
std::vector<std::string> root_data::getUserStatePath(const std::string &pk)
{
    std::vector<std::string> p;
    p.push_back("s");
    appendRelativeInternalPath(p, pk, 3);
    p.push_back(pk);
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

std::vector<NODE_id> root_data::getNodesNames()
{

    std::vector<NODE_id> v;
    auto nodes = this->getLeafNoCreate("n", db.get());
    for (auto &z : nodes->children_hashes)
    {
        NODE_id n;
        n.container = z.first;
        v.push_back(n);
    }
    return v;
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
    cc->setDirty(bc);
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
