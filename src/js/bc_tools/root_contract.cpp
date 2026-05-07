

#include <string>
#include <gmp.h>
#include <iostream>
#include <vector>
#include <string>
#include "ioBuffer.h"
#include "ghash.h"
#include <sys/stat.h>
#include "root_contract.h"
#include "msg.h"
#include "QUORUM.h"

std::vector<data_base* (*)(Cellable*)> db_constructors = {
    +[](Cellable *p) -> data_base* { return new bc_contract(p); },
    +[](Cellable *p) -> data_base* { return new bc_user(p); },
    +[](Cellable *p) -> data_base* { return new bc_user_state(p); },
    +[](Cellable *p) -> data_base* { return new bc_node(p); },
    +[](Cellable *p) -> data_base* { return new bc_values(p); },
    +[](Cellable *p) -> data_base* { return new bc_epoch(p); }
};


bool root_data::verify_lider_certificate(const REF_getter<MsgEvent::LeaderCertificate>& lc)
{
    /// проверка сертификата лидера
    {
        MUTEX_INSPECTOR;
        std::vector<blst_cpp::PublicKey> agg_pk;
        BigInt stake;
        for(auto &z: lc->nodes)
        {
            auto n=this->getNode(z,NULL);
            agg_pk.push_back(n->bls_pk);
            stake+=n->total_stake;
        }
        if(stake.toDouble() < this->getValues(NULL)->total_staked.toDouble() * QUORUM)
            throw CommonError("if(stake.toDouble() < root->getValues(NULL)->total_staked.toDouble() * QUORUM)");
        if(!lc->agg_sig.verify(agg_pk,blake2b_hash(lc->heart_beat->getBuffer()).container))
        {
            return false;
        }

    }

    return true;
}


REF_getter<Cellable> getByPathOrCreate(REF_getter<Cellable> cur, const std::vector<std::string>& v, IDatabase* db, const REF_getter<fee_calcer>& bc)
{
    cur->accessed=true;
    for(auto &z:v)
    {
        cur=cur->getLeafOrCreate(z,db,bc);
        cur->accessed=true;
    }
    return cur;
}
REF_getter<Cellable> getByPathNoCreate(REF_getter<Cellable> cur, const std::vector<std::string>& v, IDatabase* db, const REF_getter<fee_calcer>& bc)
{
    cur->accessed=true;
    for(auto &z:v)
    {
        cur=cur->getLeafNoCreate(z,db,bc);
        if(!cur.valid())
            return NULL;
        cur->accessed=true;
    }
    return cur;
}


std::vector<std::string> root_data::getContractPath(const std::string &name)
{
    std::vector<std::string> p;
    p.push_back("c");
    appendRelativeInternalPath(p,name,3);
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

REF_getter<bc_contract> root_data::getContract(const std::string &name, const REF_getter<fee_calcer>& bc)
{
    MUTEX_INSPECTOR;
    auto v=getContractPath(name);
    auto cc=getByPathNoCreate(this,v, db.get(),bc);
    if(!cc.valid())
        return NULL;
    if(!cc->data.valid())
        throw CommonError("if(!cc->data.valid())");
    return dynamic_cast<bc_contract*> (cc->data.get());
}

REF_getter<bc_contract> root_data::addContract(const std::string &name, const REF_getter<fee_calcer>& bca)
{
    MUTEX_INSPECTOR;
    auto v=getContractPath(name);
    auto cc=getByPathOrCreate(this,v,db.get(),bca);
    if(!cc.valid())
        throw CommonError("if(!cc.valid())");
    if(cc->data.valid())
        throw CommonError("if(cc->data.valid())");
    if(cc->payload_.size())
        throw CommonError("if(cc->payload.size())");
    REF_getter<bc_contract> bc=new bc_contract(cc.get());
    cc->data=bc.get();
    cc->payload_ctor_idx=hsh::bc_contract;
    cc->setDirty();
    return bc;
}
// void  root_data::setContract(const std::string &name, const REF_getter<fee_calcer>& bca,const REF_getter<bc_contract> &c)
// {
//     MUTEX_INSPECTOR;
//     auto v=getContractPath(name);
//     auto cc=getByPathOrCreate(this,v,db.get(),bca);
//     if(!cc.valid())
//         throw CommonError("if(!cc.valid())");
//     // if(cc->data.valid())
//     //     throw CommonError("if(cc->data.valid())");
//     // if(cc->payload.size())
//     //     throw CommonError("if(cc->payload.size())");
//     // REF_getter<bc_contract> bc=new bc_contract;
//     cc->data=c.get();
//     cc->payload_ctor_idx=hsh::bc_contract;
//     cc->setDirty();
// }
REF_getter<bc_epoch> root_data::getEpoch(const REF_getter<fee_calcer>& bc)
{
    MUTEX_INSPECTOR;
    auto r=this;
    auto l=r->getLeafOrCreate("ep",db.get(),bc);
    if(!l.valid()) throw CommonError("if(!l.valid())");
    if(l->data.valid())
    {
        return dynamic_cast<bc_epoch*>(l->data.get());
    }
    if(l->payload_.size())
        throw CommonError("if(l->payload_.size())");

    REF_getter<bc_epoch> v=new bc_epoch(l.get());
    l->data=v.get();
    l->payload_ctor_idx=hsh::bc_epoch;
    return v;

}
// void root_data::setEpoch(const REF_getter<bc_epoch> &v)
// {
//     MUTEX_INSPECTOR;
//     auto r=this;
//     auto l=r->getLeafOrCreate("ep",db.get(),NULL);
//     if(!l.valid()) throw CommonError("if(!l.valid())");
//     l->data=v.get();
//     l->payload_ctor_idx=hsh::bc_epoch;
//     l->setDirty();

// }
// void root_data::setValues(const REF_getter<fee_calcer>& bc, const REF_getter<bc_values> &v)
// {
//     MUTEX_INSPECTOR;
//     auto r=this;
//     auto l=r->getLeafOrCreate("v",db.get(),bc);
//     if(!l.valid()) throw CommonError("if(!l.valid())");
//     // if(l->data.valid())
//     // {
//     //     return dynamic_cast<bc_values*>(l->data.get());
//     // }
//     // if(l->payload.size())
//     //     throw CommonError("if(l->payload.size())");

//     // REF_getter<bc_values> v=new bc_values;
//     l->data=v.get();
//     l->payload_ctor_idx=hsh::bc_values;
//     l->setDirty();
//     // return v;

// }

REF_getter<bc_values> root_data::getValues(const REF_getter<fee_calcer>& bc)
{
    MUTEX_INSPECTOR;
    auto r=this;
    auto l=r->getLeafOrCreate("v",db.get(),bc);
    if(!l.valid()) throw CommonError("if(!l.valid())");
    if(l->data.valid())
    {
        return dynamic_cast<bc_values*>(l->data.get());
    }
    if(l->payload_.size())
        throw CommonError("if(l->payload_.size())");

    REF_getter<bc_values> v=new bc_values(l.get());
    l->data=v.get();
    l->payload_ctor_idx=hsh::bc_values;
    return v;
}

std::vector<std::string> root_data::getUserPath(const std::string &pk)
{
    std::vector<std::string> p;
    p.push_back("u");
    appendRelativeInternalPath(p,pk,3);
    p.push_back(pk);
    return p;

}
std::vector<std::string> root_data::getUserStatePath(const std::string &pk)
{
    std::vector<std::string> p;
    p.push_back("s");
    appendRelativeInternalPath(p,pk,3);
    p.push_back(pk);
    return p;

}

REF_getter<bc_user> root_data::getUser(const std::string &pk, const REF_getter<fee_calcer>& bc)
{
    MUTEX_INSPECTOR;
    auto v=getUserPath(pk);
    auto cc=getByPathOrCreate(this,v,db.get(),bc);
    if(!cc.valid())
        return NULL;
    if(cc->data.valid())
    {
        return dynamic_cast<bc_user*>(cc->data.get());
    }
    if(cc->payload_.size())
    throw CommonError("if(cc->payload_.size())");
    // else throw CommonError("if(cc->data.valid())");
    REF_getter<bc_user> u=new bc_user(cc.get());
    cc->data=u.get();
    cc->payload_ctor_idx=hsh::bc_user;
    return u;

}
REF_getter<bc_user_state> root_data::getUserState(const std::string &pk, const REF_getter<fee_calcer>& bc)
{
    MUTEX_INSPECTOR;
    auto v=getUserStatePath(pk);
    auto cc=getByPathOrCreate(this,v,db.get(),bc);
    if(!cc.valid())
        return NULL;
    if(cc->data.valid())
    {
        return dynamic_cast<bc_user_state*>(cc->data.get());
    }
    // else throw CommonError("if(cc->data.valid())");
    if(cc->payload_.size())
        throw CommonError("if(cc->payload_.size())");

    REF_getter<bc_user_state> u=new bc_user_state(cc.get());
    cc->data=u.get();
    cc->payload_ctor_idx=hsh::bc_user_state;
    return u;

}

// void root_data::addUser(const std::string& pk, const REF_getter<fee_calcer>& bc, const REF_getter<bc_user> &u)
// {
//     MUTEX_INSPECTOR;
//     auto v=getUserPath(pk);
//     auto cc=getByPathOrCreate(this,v,db.get(),bc);
//     if(!cc.valid())
//         throw CommonError("if(!cc.valid())");
//     if(cc->data.valid())
//         throw CommonError("if(cc->data.valid())");
//     if(cc->payload.size())
//         throw CommonError("if(cc->payload.size())");
//     cc->data=u.get();
//     cc->payload_ctor_idx=hsh::bc_user;
//     u->pkbin_еd=pk;
//     cc->setDirty();
// }
// void root_data::setUser(const std::string& pk, const REF_getter<fee_calcer>& bc, const REF_getter<bc_user> &u)
// {
//     MUTEX_INSPECTOR;
//     auto v=getUserPath(pk);
//     auto cc=getByPathNoCreate(this,v,db.get(),bc);
//     if(!cc.valid())
//         throw CommonError("if(!cc.valid())");
//     if(cc->data.valid())
//         throw CommonError("if(cc->data.valid())");
//     if(cc->payload.size())
//         throw CommonError("if(cc->payload.size())");
//     cc->data=u.get();
//     cc->payload_ctor_idx=hsh::bc_user;
//     cc->setDirty();
//     u->pkbin_еd=pk;
// }

// void root_data::addUserState(const std::string& pk, const REF_getter<fee_calcer>& bc, const REF_getter<bc_user_state> &u)
// {
//     MUTEX_INSPECTOR;
//     auto v=getUserStatePath(pk);
//     auto cc=getByPathOrCreate(this,v,db.get(),bc);
//     if(!cc.valid())
//         throw CommonError("if(!cc.valid())");
//     if(cc->data.valid())
//         throw CommonError("if(cc->data.valid())");
//     if(cc->payload.size())
//         throw CommonError("if(cc->payload.size())");
//     cc->data=u.get();
//     cc->payload_ctor_idx=hsh::bc_user_state;
//     cc->setDirty();
// }
// void root_data::setUserState(const std::string& pk, const REF_getter<fee_calcer>& bc, const REF_getter<bc_user_state> &u)
// {
//     MUTEX_INSPECTOR;
//     auto v=getUserStatePath(pk);
//     auto cc=getByPathNoCreate(this,v,db.get(),bc);
//     if(!cc.valid())
//         throw CommonError("if(!cc.valid())");
//     // if(cc->data.valid())
//     //     throw CommonError("if(cc->data.valid())");
//     // if(cc->payload.size())
//     //     throw CommonError("if(cc->payload.size())");
//     cc->data=u.get();
//     cc->payload_ctor_idx=hsh::bc_user_state;
//     cc->setDirty();
// }


std::vector<NODE_id> root_data::getNodesNames( const REF_getter<fee_calcer>& bc)
{

    std::vector<NODE_id>v;
    auto nodes=this->getLeafNoCreate("n",db.get(),bc);
    for(auto &z: nodes->children_hashes)
    {
        NODE_id n;
        n.container=z.first;
        v.push_back(n);
    }
    return v;
}
REF_getter<bc_node> root_data::addNode(const NODE_id &name, const REF_getter<fee_calcer>& bc)
{
    MUTEX_INSPECTOR;

    std::vector<std::string> v=getNodePath(name.container);
    // v.push_back("n");
    // v.push_back(name.container);
    auto cc=getByPathOrCreate(this,v,db.get(),bc);

    if(cc->data.valid())
        throw CommonError("if(cc->data.valid())");
    if(cc->payload_.size())
        throw CommonError("if(cc->payload.size())");
    REF_getter<bc_node> n=new bc_node(cc.get());
    cc->data=n.get();
    cc->payload_ctor_idx=hsh::bc_node;
    cc->setDirty();
    return n;
}
// void root_data::setNode(const NODE_id &name, const REF_getter<fee_calcer>& bc, const REF_getter<bc_node> &n)
// {
//     MUTEX_INSPECTOR;

//     std::vector<std::string> v=getNodePath(name.container);
//     // v.push_back("n");
//     // v.push_back(name.container);
//     auto cc=getByPathOrCreate(this,v,db.get(),bc);

//     if(!cc->data.valid())
//         throw CommonError("!if(cc->data.valid())");
//     // if(cc->payload.size())
//     //     throw CommonError("if(cc->payload.size())");

//     // REF_getter<bc_node> u=new bc_node;
//     cc->data=n.get();
//     cc->payload_ctor_idx=hsh::bc_node;
//     cc->setDirty();
// }

REF_getter<bc_node> root_data::getNode(const NODE_id &name, const REF_getter<fee_calcer>& bc)
{
    MUTEX_INSPECTOR;
    std::vector<std::string> v=getNodePath(name.container);
    // v.push_back("n");
    // v.push_back(name.container);

    auto cc=getByPathNoCreate(this,v,db.get(),bc);
    if(!cc.valid())
        return NULL;
    if(cc->data.valid())
        return dynamic_cast<bc_node*>(cc->data.get());
    else throw CommonError("if(cc->data.valid())");

}


REF_getter<root_data> getRoot(IDatabase* db)
{
    MUTEX_INSPECTOR;
    // logErr2("if(!root.valid())");

    REF_getter<root_data> r=new root_data(db);
    r->accessed=true;
    // std::string r;
    THASH_id root_hash;
    std::string root_cell;
    int err1=db->get_cell("#root_hash#",&root_hash.container);
    if(!err1)
    {
        int err=db->get_cell("#root#",&root_cell);
        if(!err)
        {
            if(blake2b_hash(root_cell)!=root_hash)
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
    else logErr2("cannot read #root_hash#");


    return r;
}
