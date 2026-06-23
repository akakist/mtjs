#pragma once
#include <string>
#include "blst_cp.h"
#include "cellable.h"
#include <fcntl.h>
#include "ioBuffer.h"
#include "bigint.h"
#include "base16.h"
#include "NODE_id.h"
#include "hsh.h"
#include "md/md_LeaderCertificate.h"
#include <xyjson.h>
#include <sstream>
#include "fee_calcer.h"
#include "nodeElement.h"
#include "ADDRESS_id.h"
// #include <ostringstream>

struct bc_contract:  public data_base
{

    bc_contract(Cellable *p):data_base(hsh::bc_contract,p, 0,-1) {}
    std::string name_;
    ADDRESS_id  owner;
    std::string src;
    void pack(outBuffer&b) const final
    {
        data_base::pack(b);
        b<<1;
        b<<name_<<owner<<src;

    }
    void unpack(inBuffer&b) final
    {
        data_base::unpack(b);
        auto v=b.get_PN();
        b>>name_>>owner>>src;

    }
    std::string dump() final;
};
struct bc_user: public data_base
{

    bc_user(Cellable* p): data_base(hsh::bc_user,p, 0,-1) {
    }
    ADDRESS_id address;
    // std::map<NODE_id /*nodeName*/, BigInt /*stake*/> my_stakes;
    // std::set<NODE_id> nodes;
    // std::set<std::string> contracts;

    void pack(outBuffer& o) const final
    {
        data_base::pack(o);
        o<<1;
        o<<address
         // <<my_stakes<<nodes<<contracts
         ;
    }
    void unpack(inBuffer& o) final
    {
        data_base::unpack(o);
        auto v=o.get_PN();
        o>>address
         // >>my_stakes>>nodes>>contracts
         ;
    }
    std::string dump() final
    {
        std::ostringstream o;
        o<<"ADDRESS: "  << base16::encode(address.addr) << std::endl;
        o<< std::endl;


        o<< std::endl;


        return o.str();
    }

};
struct bc_user_state: public data_base
{

    bc_user_state(Cellable* p): data_base(hsh::bc_user_state,p, 0,-1) {
        nonce=0;
        balance=0;
    }
    private:
    BigInt balance;
    uint64_t nonce;
    public:
    BigInt getBalance()
    {
        M_LOCK(parent->mx);
        return balance;
    }
    void addBalance(const BigInt &n)
    {
        M_LOCK(parent->mx);
        balance+=n;
    }
    void setBalance(const BigInt &n)
    {
        M_LOCK(parent->mx);
        balance=n;
    }
    void subBalance(const BigInt &n)
    {
        M_LOCK(parent->mx);
        balance-=n;
    }
    uint64_t getNonce()
    {
        M_LOCK(parent->mx);
        return nonce;
    }
    void incNonce()
    {
        M_LOCK(parent->mx);
        nonce+=1;
    }
    void pack(outBuffer& o) const final
    {
        data_base::pack(o);
        o<<1;
        o<<balance<<nonce;
    }
    void unpack(inBuffer& o) final
    {
        data_base::unpack(o);
        auto v=o.get_PN();
        o>>balance>>nonce;
    }
    std::string dump() final;

};

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
    std::map<ADDRESS_id /*user*/, BigInt> stakes;
    int missed_rounds = 0;
    public:
    NodeElement getElement()
    {
        NodeElement n;
        M_LOCK(parent->mx);
        n.ip=ip;
        n.missed_rounds=missed_rounds;
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
    void inc_missed_rounds()
    {
        M_LOCK(parent->mx);
        missed_rounds++;
    }
    void reset_missed_rounds()
    {
        M_LOCK(parent->mx);
        missed_rounds=0;
    }
    int get_missed_rounds()
    {
        M_LOCK(parent->mx);
        return missed_rounds;
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
    BigInt get_full_stake()
    {
        BigInt ret=0;
        M_LOCK(parent->mx);
        for(auto &z: stakes)
        {
            ret+=z.second;
        }
        return ret;
    }
    BigInt get_user_stake(const ADDRESS_id& user)
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
    void add_stake(const ADDRESS_id& user, const BigInt &amount)
    {
        M_LOCK (parent->mx);
        stakes[user]+=amount;
    }
    void sub_stake(const ADDRESS_id& user, const BigInt &amount)
    {
        M_LOCK (parent->mx);
        stakes[user]-=amount;
    }


    void pack(outBuffer& o)  const final
    {
        data_base::pack(o);
        o<<1;
        o<<name_<<owner_address<<bls_pk<<ed_pk<<ip<<stakes<<missed_rounds;
    }
    void unpack(inBuffer& o) final
    {
        data_base::unpack(o);
        auto v=o.get_PN();

        o>>name_>>owner_address>>bls_pk>>ed_pk>>ip>>stakes>>missed_rounds;
    }
    std::string dump() final;

};

struct bc_values: public data_base
{


bc_values(Cellable *p): data_base(hsh::bc_values,p,0,-1) {
        fees["contract_deploy"]=BigInt(5000);
        fees["contract_transfer"]=BigInt(1000);
        fees["node_create"]=BigInt(20000);
        fees["node_update"]=BigInt(10000);
        fees["node_enable"]=BigInt(5000);
        fees["node_unstake"]=BigInt(2000);
        fees["node_stake"]=BigInt(2000);
        fees["mint"]=BigInt(95);
        fees["transfer"]=BigInt(1000);
    }
    std::map<std::string,BigInt> fees;
    std::set<ADDRESS_id> emitters_bin;
    BigInt getFee(const std::string &fee_type) const
    {
        auto it=fees.find(fee_type);
        if(it!=fees.end())
            return it->second;
        throw CommonError("fee '%s' not found", fee_type.c_str());
        return BigInt(0);
    }
    void pack(outBuffer& o) const final
    {
        // cost.pack(o);
        o<<1;
        o<<fees<<emitters_bin;
    }
    void unpack(inBuffer& o) final
    {
        // cost.unpack(o);
        auto v=o.get_PN();

        o>>fees>>emitters_bin;
    }
    std::string dump() final;

};

struct bc_epoch: public data_base
{

    bc_epoch(Cellable* p): data_base(hsh::bc_epoch,p,0,-1) {
        epoch.container=0;
    }
    EPOCH_id epoch;
    std::string prev_lc;
    void pack(outBuffer& o) const final
    {
        // cost.pack(o);
        o<<1;
        o<<epoch;
        o<<prev_lc;
    }
    void unpack(inBuffer& o) final
    {
        // cost.unpack(o);
        auto v=o.get_PN();
        o>>epoch;
        o>> prev_lc;
    }
    std::string dump() final;

};

REF_getter<Cellable> getByPathOrCreate(REF_getter<Cellable> cur, const std::vector<std::string>& v, IDatabase* db);
REF_getter<Cellable> getByPathNoCreate(REF_getter<Cellable> cur, const std::vector<std::string>& v, IDatabase* db);


struct root_data: public Cellable
{

    REF_getter<IDatabase> db;
    root_data(IDatabase *db_): Cellable(nullptr,"r"),db(db_)
    {
    }
    bool verify_leader_certificate(const REF_getter<MsgData::LeaderCertificate>& lc);

    std::vector<std::string> getContractPath(const std::string &name);
    std::vector<std::string> getNodePath(const std::string &name);
    std::vector<std::string> getUserPath(const ADDRESS_id &addr);
    std::vector<std::string> getUserStatePath(const ADDRESS_id &addr);

    REF_getter<bc_contract> getContract(const std::string &name);
    REF_getter<bc_contract> addContract(const std::string &name, const REF_getter<fee_calcer>& bca, const EPOCH_id& epoch);

    REF_getter<bc_values> getValues();
    REF_getter<bc_values> checkValues();

    REF_getter<bc_epoch> getEpoch();



    REF_getter<bc_user> getUser(const ADDRESS_id &pk);

    REF_getter<bc_user_state> getUserState(const ADDRESS_id &pk);
    REF_getter<bc_user_state>   checkUserState(const ADDRESS_id &pk);


    // std::vector<NODE_id> getNodesNames();
    std::vector<REF_getter<bc_node>> getAllNodes();


    REF_getter<bc_node> getNode(const NODE_id &name);
    REF_getter<bc_node> addNode(const NODE_id &name, const REF_getter<fee_calcer>& bc, const EPOCH_id& epoch);



};
REF_getter<root_data> getRoot(IDatabase* db);

