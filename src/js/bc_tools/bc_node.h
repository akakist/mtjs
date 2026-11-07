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
    std::map<ADDRESS_id /*user*/, int64_t> stakes;
    bool enabled=true;
public:
    int missed_rounds=0;
    int64_t answered_rounds=0;   // ← новое: серия ответов подряд (fix_missed_rounds)
    size_t size() const
    {
        
        size_t r=size_();
        r+=name_.container.size();
        r+=owner_address.addr.size();
        r+=48;
        r+=ed_pk.size();
        r+=ip.size();
        for(auto& z: stakes)
        {
            r+=z.first.addr.size();
            r+=sizeof(int64_t);
        }
        r+=sizeof(missed_rounds);
        r+=sizeof(answered_rounds);
        return r;
    }
    void setEnabled(bool _enabled)
    {
        
        enabled=_enabled;
        missed_rounds=0;
    }
    bool isEnabled()
    {
        
        return enabled;
    }
    NodeElement getElement()
    {
        NodeElement n;
        
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
        
        return ed_pk;
    }
    std::string get_ip()
    {
        
        return ip;
    }
    void set_ip(const std::string& _ip)
    {
        
        ip=_ip;
    }
    ADDRESS_id get_owner() const
    {
        
        return owner_address;
    }
    std::string ip_port()
    {
        
        return ip;
    }
    NODE_id getName()
    {
        
        return name_;
    }
    int64_t get_full_stake() const
    {
        int64_t ret=0;
        
        for(auto &z: stakes)
        {
            ret+=z.second;
        }
        return ret;
    }
    int64_t get_user_stake(const ADDRESS_id& user) const
    {
        
        auto it = stakes.find(user);
        if (it != stakes.end())
        {
            return it->second;
        }
        return 0;
    }
    blst_cpp::PublicKey get_bls_pk() const
    {
        
        return bls_pk;
    }
    std::map<ADDRESS_id, int64_t> getStakes() const
    {
        
        return stakes;
    }
    void init(const NODE_id& name, const ADDRESS_id &_owner_address, const blst_cpp::PublicKey &_bls_pk,
              const std::string &_ed_pk, const std::string& _ip)
    {
        
        name_=name;
        owner_address=_owner_address;
        bls_pk=_bls_pk;
        ed_pk=_ed_pk;
        ip=_ip;
        enabled=true;
    }
    void add_stake(const ADDRESS_id& user, const int64_t &amount)
    {
        
        stakes[user]+=amount;
    }
    void sub_stake(const ADDRESS_id& user, const int64_t &amount)
    {
        
        stakes[user]-=amount;
    }


    void pack(outBuffer& o)  const final
    {
        data_base::pack(o);
        o<<1;
        o<<name_<<owner_address<<bls_pk<<ed_pk<<ip<<stakes<<enabled<<missed_rounds<<answered_rounds;
    }
    void unpack(inBuffer& o) final
    {
        data_base::unpack(o);
        auto v=o.get_PN();

        o>>name_>>owner_address>>bls_pk>>ed_pk>>ip>>stakes>>enabled>>missed_rounds>>answered_rounds;
    }

};
