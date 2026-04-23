

#include <string>
#include <gmp.h>
#include <iostream>
#include <vector>
#include <string>
#include "ioBuffer.h"
// #include "auto_mpz_t.h"
#include "ghash.h"
#include <sys/stat.h>
#include "root_contract.h"
// #include "nodeService.h"
#include "msg.h"
#include "QUORUM.h"
// bc_contract,
// bc_user,
// bc_node,
// bc_values,
// auto d1=[](){return new bc_contract;}
// data_base* create_bc() { return new bc_contract; }
// data_base* create_user() { return new bc_user; }
// std::vector< data_base* (*)()> db_constructors={
//     create_bc, create_user
// };

std::vector<data_base* (*)()> db_constructors = {
    +[]() -> data_base* { return new bc_contract; },
    +[]() -> data_base* { return new bc_user; },
    +[]() -> data_base* { return new bc_node; },
    +[]() -> data_base* { return new bc_values; }
    // +[]() -> data_base* { return new bc_nick; }
};


bool root_data::verify_lider_certificate(const msg::leader_certificate& lc)
{
    /// проверка сертификата лидера
    {
        MUTEX_INSPECTOR;
        bls::PublicKey agg_pk;
        agg_pk.clear();
        BigInt stake;
        for(auto &z: lc.nodes)
        {
            auto n=this->getNode(z,NULL);
            agg_pk.add(n->bls_pk);
            stake+=n->total_stake;
        }
        if(stake.toDouble() < this->getValues(NULL)->total_staked.toDouble() * QUORUM)
            throw CommonError("if(stake.toDouble() < root->getValues(NULL)->total_staked.toDouble() * QUORUM)");
        if(!lc.agg_sig.verify(agg_pk,blake2b_hash(lc.payload_heart_beat).container))
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
    if(cc->payload.size())
        throw CommonError("if(cc->payload.size())");
    REF_getter<bc_contract> bc=new bc_contract;
    cc->data=bc.get();
    cc->payload_ctor_idx=hsh::bc_contract;
    return bc;
}

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
    if(l->payload.size())
        throw CommonError("if(l->payload.size())");

    REF_getter<bc_values> v=new bc_values;
    l->data=v.get();
    l->payload_ctor_idx=hsh::bc_values;
    return v;
}

std::vector<std::string> root_data::getUserPath(const std::string &nick)
{
    std::vector<std::string> p;
    p.push_back("u");
    appendRelativeInternalPath(p,nick,3);
    p.push_back(nick);
    return p;

}

REF_getter<bc_user> root_data::getUser(const std::string &nick, const REF_getter<fee_calcer>& bc)
{
    MUTEX_INSPECTOR;
    auto v=getUserPath(nick);

    auto cc=getByPathNoCreate(this,v,db.get(),bc);
    if(!cc.valid())
        return NULL;
    if(cc->data.valid())
    {
        return dynamic_cast<bc_user*>(cc->data.get());
    }
    else throw CommonError("if(cc->data.valid())");
}

REF_getter<bc_user> root_data::addUser(const std::string &nick, const std::string& pk, const REF_getter<fee_calcer>& bc)
{
    MUTEX_INSPECTOR;
    auto v=getUserPath(nick);
    auto cc=getByPathOrCreate(this,v,db.get(),bc);
    if(!cc.valid())
        throw CommonError("if(!cc.valid())");
    if(cc->data.valid())
        throw CommonError("if(cc->data.valid())");
    if(cc->payload.size())
        throw CommonError("if(cc->payload.size())");
    REF_getter<bc_user> u=new bc_user;
    cc->data=u.get();
    cc->payload_ctor_idx=hsh::bc_user;
    u->nick=nick;
    u->pkbin=pk;
    return u;
}


/////////////////
// std::vector<std::string> root_data::getNickPath(const std::string &pk)
// {
//     std::vector<std::string> p;
//     p.push_back("nicks");
//     appendRelativeInternalPath(p,pk,3);
//     p.push_back(base62::encode(pk));
//     return p;

// }

// REF_getter<bc_nick> root_data::getNick(const std::string &pk, const REF_getter<fee_calcer>& bc)
// {
//     MUTEX_INSPECTOR;
//     auto v=getNickPath(pk);

//     auto cc=getByPathNoCreate(this,v,db.get(),bc);
//     if(!cc.valid())
//         return NULL;
//     if(cc->data.valid())
//     {
//         return dynamic_cast<bc_nick*>(cc->data.get());
//     }
//     else throw CommonError("if(cc->data.valid())");
// }
// REF_getter<bc_nick> root_data::addNick(const std::string &pk, const REF_getter<fee_calcer>& bc)
// {
//     MUTEX_INSPECTOR;
//     auto v=getNickPath(pk);
//     auto cc=getByPathOrCreate(this,v,db.get(),bc);
//     if(!cc.valid())
//         throw CommonError("if(!cc.valid())");
//     if(cc->data.valid())
//         throw CommonError("if(cc->data.valid())");
//     if(cc->payload.size())
//         throw CommonError("if(cc->payload.size())");
//     REF_getter<bc_nick> u=new bc_nick;
//     cc->data=u.get();
//     cc->payload_ctor_idx=hsh::bc_nick;
//     return u;
// }
