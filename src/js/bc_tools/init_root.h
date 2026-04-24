#pragma once
#include "REF.h"
#include "root_contract.h"
#include "getenv2.h"

inline void init_root(const REF_getter<root_data> &r)
{
    auto v=r->getValues(NULL);
    // u_root pk
    std::string u_root_pk=base62::decode(getenv2("k_root_ed_pk"));
    if(!v->emitters.count(u_root_pk))
        v->emitters.insert(u_root_pk);
    auto u=r->getUser(u_root_pk,NULL);
    if(!u.valid())
    {
        u=r->addUser(u_root_pk,NULL);

        u->balance=100;
    }

    std::vector<int> stakes= {100,200,300,400,500};
    int total=0;
    for(auto &z: stakes)
    {
        total+=z;
    }
    v->total_staked=total;
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
        auto n=r->getNode(name,NULL);
        if(n.valid()) continue;

        n=r->addNode(name,NULL);
        n->name=name;
        n->owner_ed_pk=u_root_pk;
        n->total_stake=stakes[i];//.from_decimal(std::to_string(stakes[i]));

        n->bls_pk.deserializeBase62Str(getenv2(keys[i].first));

        n->ed_pk=base62::decode(getenv2(keys[i].second));

        n->disabled_manual=false;
        n->disabled_offline=false;
        n->ip="127.0.0.1:"+std::to_string(2300+i);
    }


}
