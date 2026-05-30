#pragma once
#include "REF.h"
#include "root_contract.h"
#include "getenv2.h"

inline void init_root(const REF_getter<root_data> &r)
{
    std::vector<int> stakes= {100,200,300,400,500};
    std::string u_root_pk_hex=getenv2("k_root_ed_pk");
    if(!r->checkValues().valid())
    {
        auto v=r->getValues();
        if(!v->emitters_hex.count(u_root_pk_hex))
            v->emitters_hex.insert(u_root_pk_hex);

        int total=0;
        for(auto &z: stakes)
        {
            total+=z;
        }
        v->total_staked=total;
        v->setDirty(NULL);
    }
    // u_root pk
    if(!r->checkUserState(u_root_pk_hex).valid())
    {
        auto u=r->getUserState(u_root_pk_hex);
        if(!u.valid())
        {
            throw CommonError("cannot find root user state");
        }
        u->balance=1000000;
        u->setDirty(NULL);

    }


    // logErr2("v->total_staked %lf",v->total_staked.toDouble());

    std::vector<std::pair<std::string, std::string>> keys=
    {
        {
            "k_n0_bls_pk",
            "k_n0_ed_pk"
        },
        {
            "k_n1_bls_pk",
            "k_n1_ed_pk"
        },
        {
            "k_n2_bls_pk",
            "k_n2_ed_pk"
        },
        {
            "k_n3_bls_pk",
            "k_n3_ed_pk"
        },
        {
            "k_n4_bls_pk",
            "k_n4_ed_pk"
        }
    };




    for(int i=0; i<5; i++)
    {
        NODE_id name;
        name.container="n"+std::to_string(i);
        auto n=r->getNode(name);
        if(n.valid()) continue;

        REF_getter<bc_node> nn=r->addNode(name,NULL);
        nn->name_=name;
        nn->owner_ed_pkhex=u_root_pk_hex;
        nn->total_stake=stakes[i];//.from_decimal(std::to_string(stakes[i]));
        nn->bls_pk.deserializeHexStr(getenv2(keys[i].first));

        nn->ed_pk=base62::decode(getenv2(keys[i].second));

        nn->ip="127.0.0.1:"+std::to_string(2300+i);
        nn->setDirty(NULL);
        // r->;

    }


}
