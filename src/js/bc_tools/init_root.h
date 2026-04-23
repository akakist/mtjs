#pragma once
#include "REF.h"
#include "root_contract.h"

inline void init_root(const REF_getter<root_data> &r)
{
    auto v=r->getValues(NULL);
    // u_root pk
    std::string u_root_pk=iUtils->hex2bin((std::string)"c0be061fd44868bae4bfa04d8d17d96834da4da0af6745f56c2802054d39fa63");
    if(!v->emitters.count("root"))
        v->emitters.insert("root");
    auto u=r->getUser("root",NULL);
    if(!u.valid())
    {
        u=r->addUser("root",u_root_pk,NULL);

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
            "1a0e1cff32df09b3c9ed0904cba95c45aa0b6034bfbed9ded624639d80aa6f0f7d3fd2fd164018076621744e3aee0305b901bf12d171ed2daa5e07034d056718",
            "fedacdc0b68ebef41dbe45bc2f4ba679b3ebdc907f3a473733daa180629d755d"
        },
        {
            "1673ec4f53dbc0b8cb43318f05e108631c56d1e05a5c52979fa84b0e3bc42b21a29b99fd2846fd1b2dc1839f6ed5b9cde92eac945786a7d9363bfa1859a3fba0",
            "b813d310e8c58f64a543377992bacd92771ac1419dbb82cab6dfeff4bbf1079e"
        },
        {
            "84f85439d4dbe89325bc1bf3352524dc79bbe9f7160dfb62d2904f615ef8981f0fefbed546e9ab5b3fb2e6e74d64f6275a291020b6bf1a626bb1abb662fefa91",
            "b7b87ff73e7d530f9e8f32b0a7e8d07a80efa6ef337415f299992cc314781242"
        },
        {
            "47720abe78d062c447995e97139fb51b711c0ba2b4a210323f75519a74bc050a817660172873b1624f9b99c5b3a1c091e1e2b1f6d977077d303df90b0ffc168d",
            "436c0ebc2344111d934caab8a21da257055d2958890f65456c0c7f11c2a2f002"
        },
        {
            "11e7a5dd5e47cd5c1e56c4fcfcaaa38e10c6d84f120d15b565fce603a633f11236e092e02fd18fe7852598505c0918499f74a6be011cfbdc80639d1675f9a895",
            "2502bcbd9d2df88e7c6244a3f0e09c7a00d50aa04b622222f567d21775eb5859"
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
        n->owner="root";
        n->total_stake=stakes[i];//.from_decimal(std::to_string(stakes[i]));
        n->bls_pk.deserializeHexStr(keys[i].first);
        n->ed_pk=iUtils->hex2bin(keys[i].second);
        n->disabled_manual=false;
        n->disabled_offline=false;
        n->ip="127.0.0.1:"+std::to_string(2300+i);
    }

}
