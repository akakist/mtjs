#pragma once
#include "cellable.h"
#include "NODE_id.h"
#include "ADDRESS_id.h"
#include "blst_cp.h"
#include "nodeElement.h"
struct bc_node: public data_base
{


    bc_node(Cellable *p):data_base(hsh::bc_node,p,0,-1) {
    }
    private:
    NODE_id name_;
    ADDRESS_id owner_address;
    blst_cpp::PublicKey bls_pk;
    std::string ed_pk;
    std::string ip;
    std::map<ADDRESS_id /*user*/, uint64_t> stakes;
    public:
    NodeElement getElement()
    {
        NodeElement n;
        M_LOCK(parent->mx);
        n.ip=ip;
        n.name=name_;
        n.stake_A=0;
        for(auto &z: stakes)
        {
            n.stake_A+=z.second;
        }
        return n;
    }
    std::string get_ed_pk()
    {
        M_LOCK(parent->mx);
        return ed_pk;
    }
    std::string get_ip()
    {
        M_LOCK(parent->mx);
        return ip;
    }
    void set_ip(const std::string& _ip)
    {
        M_LOCK(parent->mx);
        ip=_ip;
    }
    ADDRESS_id get_owner()
    {
        M_LOCK(parent->mx);
        return owner_address;
    }
     std::string ip_port()
    {
        M_LOCK(parent->mx);
        return ip;
    }
    NODE_id getName()
    {
        M_LOCK(parent->mx);
        return name_;
    }
    uint64_t get_full_stake()
    {
        uint64_t ret=0;
        M_LOCK(parent->mx);
        for(auto &z: stakes)
        {
            ret+=z.second;
        }
        return ret;
    }
    uint64_t get_user_stake(const ADDRESS_id& user)
    {
        M_LOCK(parent->mx);
        auto it = stakes.find(user);
        if (it != stakes.end())
        {
            return it->second;
        }
        return 0;
    }
    blst_cpp::PublicKey get_bls_pk()
    {
        M_LOCK(parent->mx);
        return bls_pk;
    }
    void init(const NODE_id& name, const ADDRESS_id &_owner_address, const blst_cpp::PublicKey &_bls_pk,
    const std::string &_ed_pk, const std::string& _ip)
    {
        M_LOCK (parent->mx);
        name_=name;
        owner_address=_owner_address;
        bls_pk=_bls_pk;
        ed_pk=_ed_pk;
        ip=_ip;
    }
    void add_stake(const ADDRESS_id& user, const uint64_t &amount)
    {
        M_LOCK (parent->mx);
        stakes[user]+=amount;
    }
    void sub_stake(const ADDRESS_id& user, const uint64_t &amount)
    {
        M_LOCK (parent->mx);
        stakes[user]-=amount;
    }


    void pack(outBuffer& o)  const final
    {
        data_base::pack(o);
        o<<1;
        o<<name_<<owner_address<<bls_pk<<ed_pk<<ip<<stakes;
    }
    void unpack(inBuffer& o) final
    {
        data_base::unpack(o);
        auto v=o.get_PN();

        o>>name_>>owner_address>>bls_pk>>ed_pk>>ip>>stakes;
    }

};
