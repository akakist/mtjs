#include "init_root.h"
#include "getenv2.h"
void init_root(const REF_getter<root_data> &r)
{
    MUTEX_INSPECTOR;
    std::vector<int> stakes;
    for(int i=0; i<20; i++)
    {
        stakes.push_back(100*i);
    }
    std::string u_root_pk=base16::decode(getenv2("k_root_ed_pk"));
    if(!r->checkValues().valid())
    {
        auto v=r->getValues();
        if(!v->emitters_bin.count(u_root_pk))
            v->emitters_bin.insert(u_root_pk);

        int total=0;
        for(auto &z: stakes)
        {
            total+=z;
        }
        v->total_staked=total;
        v->setDirty();
    }
    // u_root pk
    if(!r->checkUserState(u_root_pk).valid())
    {
        auto u=r->getUserState(u_root_pk);
        if(!u.valid())
        {
            throw CommonError("cannot find root user state");
        }
        u->addBalance(1000000);
        u->setDirty();

    }

    std::vector<std::pair<std::string, std::string>> keys;
    for (size_t i = 0; i < 20; i++)
    {
        keys.push_back(
        {
            "k_node"+std::to_string(i)+"_bls_pk",
            "k_node"+std::to_string(i)+"_ed_pk"
        }
        );
    }


    for(int i=0; i<20; i++)
    {
        NODE_id name;
        name.container="n"+std::to_string(i);
        auto n=r->getNode(name);
        if(n.valid()) continue;

        REF_getter<bc_node> nn=r->addNode(name,NULL);
        blst_cpp::PublicKey bls_pk;
        bls_pk.deserializeHexStr(getenv2(keys[i].first));
        nn->init(name, u_root_pk, bls_pk, base16::decode(getenv2(keys[i].second)), "127.0.0.1:"+std::to_string(2300+i));
        // "
        // nn->name_=name;
        // nn->owner_ed_pk=u_root_pk;
        // nn->total_stake=stakes[i];//.from_decimal(std::to_string(stakes[i]));
        // nn->bls_pk.deserializeHexStr(getenv2(keys[i].first));

        // nn->ed_pk=base16::decode(getenv2(keys[i].second));

        // nn->ip="127.0.0.1:"+std::to_string(2300+i);
        nn->setDirty();
        // r->;

    }


}
