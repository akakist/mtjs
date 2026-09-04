#include "init_root.h"
#include "getenv2.h"
#include "IDatabase.h"
void init_root(IDatabase* db)
{
    MUTEX_INSPECTOR;
        uint64_t e=0;
    std::vector<int> stakes;
    for(int i=0; i<10; i++)
    {
        stakes.push_back(100*i);
    }
    std::string u_root_pk=base16::decode(getenv2("k_root_ed_pk"));
    ADDRESS_id u_root_address;
    u_root_address.addr=blake2b_hash(u_root_pk).container;
    if(!db->checkValues(db).valid())
    {
        auto v=db->getValuesOrCreate(NULL,db);
        if(!v->emitters_bin.count(u_root_address))
            v->emitters_bin.insert(u_root_address);

        int total=0;
        for(auto &z: stakes)
        {
            total+=z;
        }
        v->setDirty(NULL);
    }
    // u_root pk
    if(!db->checkUserState(u_root_address,db).valid())
    {
        auto u=db->getAddressState(u_root_address,NULL,db);
        if(!u.valid())
        {
            throw CommonError("cannot find root user state");
        }
        {
            M_LOCK(u->parent->mx);
            u->balance+=100000000;
        }
        u->setDirty(NULL);

    }

    std::vector<std::pair<std::string, std::string>> keys;
    for (size_t i = 0; i < 10; i++)
    {
        keys.push_back(
        {
            "k_node"+std::to_string(i)+"_bls_pk",
            "k_node"+std::to_string(i)+"_ed_pk"
        }
        );
    }


    for(int i=0; i<10; i++)
    {
        NODE_id name;
        name.container="n"+std::to_string(i);
        auto n=db->getNode(name,db);
        if(n.valid()) continue;

        REF_getter<bc_node> nn=db->addNode(name,NULL,db);
        blst_cpp::PublicKey bls_pk;
        bls_pk.deserializeHexStr(getenv2(keys[i].first));
        nn->init(name, u_root_address, bls_pk, base16::decode(getenv2(keys[i].second)), "127.0.0.1:"+std::to_string(2300+i));
        nn->add_stake(u_root_address, 100*i);
        nn->setDirty(NULL);
        // r->;

    }


}
