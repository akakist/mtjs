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
// #include <ostringstream>

struct bc_contract:  public data_base
{

    bc_contract(Cellable *p):data_base(hsh::bc_contract,p) {}
    std::string name_;
    std::string owner;
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
    std::string dump() final
    {
        std::ostringstream o;
        o<<"Contract: "  << name_ << std::endl;
        o<< "Owner: " << base16::encode(owner) << std::endl;
        o<< "Src: " << src << std::endl;

        return o.str();
    }
};
struct bc_user: public data_base
{

    bc_user(Cellable* p): data_base(hsh::bc_user,p) {
    }
    std::string pkbin_еd;
    // std::map<NODE_id /*nodeName*/, BigInt /*stake*/> my_stakes;
    // std::set<NODE_id> nodes;
    // std::set<std::string> contracts;

    void pack(outBuffer& o) const final
    {
        data_base::pack(o);
        o<<1;
        o<<pkbin_еd
         // <<my_stakes<<nodes<<contracts
         ;
    }
    void unpack(inBuffer& o) final
    {
        data_base::unpack(o);
        auto v=o.get_PN();
        o>>pkbin_еd
         // >>my_stakes>>nodes>>contracts
         ;
    }
    std::string dump() final
    {
        std::ostringstream o;
        o<<"PK: "  << base16::encode(pkbin_еd) << std::endl;
        o<< std::endl;


        o<< std::endl;


        return o.str();
    }

};
struct bc_user_state: public data_base
{

    bc_user_state(Cellable* p): data_base(hsh::bc_user_state,p) {
        nonce=0;
        balance=0;
    }
    private:
    BigInt balance;
    BigInt nonce;
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
    BigInt getNonce()
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
    std::string dump() final
    {
        M_LOCK(parent->mx);
        std::ostringstream o;
        o<< "Balance: " << balance.toString() << std::endl;
        o<< "Nonce: " << nonce.toString() <<std::endl;
        o<< std::endl;
        return o.str();
    }

};

struct bc_node: public data_base
{


    bc_node(Cellable *p):data_base(hsh::bc_node,p) {
        total_stake=0;
    }
    private:
    NODE_id name_;
    std::string owner_ed_pk;
    blst_cpp::PublicKey bls_pk;
    std::string ed_pk;
    std::string ip;
    std::map<std::string /*user*/, BigInt> stakes;
    BigInt total_stake;
    int missed_rounds = 0;
    public:
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
    std::string get_owner()
    {
        M_LOCK(parent->mx);
        return owner_ed_pk;
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
    BigInt getStakes()
    {
        BigInt ret=0;
        M_LOCK(parent->mx);
        for(auto &z: stakes)
        {
            ret+=z.second;
        }
        return ret;
    }
    BigInt getStake(const std::string& user)
    {
        M_LOCK(parent->mx);
        auto it = stakes.find(user);
        if (it != stakes.end())
        {
            return it->second;
        }
        return 0;
    }
     BigInt getTotalStake()
    {
        M_LOCK(parent->mx);
        return total_stake;

    }
    BigInt stake()
    {
        M_LOCK(parent->mx);
        BigInt stake=0;
        for(auto &z: stakes)
        {
            stake+=z.second;
        }
        return stake;
    }
    blst_cpp::PublicKey get_bls_pk()
    {
        M_LOCK(parent->mx);
        return bls_pk;
    }
    void init(const NODE_id& name, const std::string &_owner_ed_pk, const blst_cpp::PublicKey &_bls_pk,
    const std::string &_ed_pk, const std::string& _ip)
    {
        M_LOCK (parent->mx);
        name_=name;
        owner_ed_pk=_owner_ed_pk;
        bls_pk=_bls_pk;
        ed_pk=_ed_pk;
        ip=_ip;
    }
    void addStake(const std::string& user, const BigInt &amount)
    {
        M_LOCK (parent->mx);
        stakes[user]+=amount;
    }
    void subStake(const std::string& user, const BigInt &amount)
    {
        M_LOCK (parent->mx);
        stakes[user]+=amount;
    }


    void pack(outBuffer& o)  const final
    {
        data_base::pack(o);
        o<<1;
        o<<name_<<owner_ed_pk<<bls_pk<<ed_pk<<ip<<stakes<<total_stake<<missed_rounds;
    }
    void unpack(inBuffer& o) final
    {
        data_base::unpack(o);
        auto v=o.get_PN();

        o>>name_>>owner_ed_pk>>bls_pk>>ed_pk>>ip>>stakes>>total_stake>>missed_rounds;
    }
    std::string dump() final
    {
        std::ostringstream o;
        o<< "Node: "<< name_.container << std::endl;
        o<< "Owner: "<<  base16::encode(owner_ed_pk) << std::endl;
        o<< "bls_pk: "<< base16::encode(bls_pk.serialize()) << std::endl;
        o<< "ed_pk: "<< base16::encode(ed_pk) << std::endl;
        o<< "ip:port: "<< ip << std::endl;
        o<< "missed rounds: "<< missed_rounds << std::endl;
        o<< "stake: "<< total_stake.toString() << std::endl;
        o<< "stakers: "<< std::endl;
        for(auto &z: stakes)
        {
            o<< "\t"<< base16::encode(z.first) << " -> " << z.second.toString() << std::endl;
        }
        return o.str();
    }

};

struct bc_values: public data_base
{

    // enum __fee_types
    // {
    //     contract_deploy,
    //     contract_transfer,
    //     node_create,
    //     node_update,
    //     node_enable_from_manual,
    //     node_enable_from_offline,
    //     nick_create,
    //     nick_transfer,
    //     cell_in_contract_create,
    //     freeze_contract,
    //     mint,
    //     unstake,
    //     stake,
    //     transfer,
    //     register_nick,

    //     // FEE_TYPE_END
    // };

    bc_values(Cellable *p): data_base(hsh::bc_values,p) {
        // fees.resize(FEE_TYPE_END);
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
    BigInt total_staked;
    std::set<std::string> emitters_bin;
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
        o<<fees<<total_staked<< emitters_bin;
    }
    void unpack(inBuffer& o) final
    {
        // cost.unpack(o);
        auto v=o.get_PN();

        o>>fees>>total_staked>>emitters_bin;
    }
    std::string dump() final
    {
        return "Values";
    }

};

struct bc_epoch: public data_base
{

    bc_epoch(Cellable* p): data_base(hsh::bc_epoch,p) {
        epoch=0;
    }
    BigInt epoch;
    REF_getter<MsgData::LeaderCertificate> prev_leader_cert;
    void pack(outBuffer& o) const final
    {
        // cost.pack(o);
        o<<1;
        o<<epoch;
        if(prev_leader_cert.valid())
        {
            o<<1;
            o<<prev_leader_cert;
        }
        else
        {
            o<<0;
        }
    }
    void unpack(inBuffer& o) final
    {
        // cost.unpack(o);
        auto v=o.get_PN();
        o>>epoch;
        int has_leader_cert=o.get_PN();
        if(has_leader_cert)
        {
            o>>prev_leader_cert;
        }
    }
    std::string dump() final
    {
        return "Values";
    }

};

REF_getter<Cellable> getByPathOrCreate(REF_getter<Cellable> cur, const std::vector<std::string>& v, IDatabase* db);
REF_getter<Cellable> getByPathNoCreate(REF_getter<Cellable> cur, const std::vector<std::string>& v, IDatabase* db);


struct root_data: public Cellable
{

    REF_getter<IDatabase> db;
    root_data(IDatabase *db_): Cellable(nullptr,"r"),db(db_)
    {
    }
    bool verify_lider_certificate(const REF_getter<MsgData::LeaderCertificate>& lc);

    std::vector<std::string> getContractPath(const std::string &name);
    std::vector<std::string> getNodePath(const std::string &name);
    std::vector<std::string> getUserPath(const std::string &pk);
    std::vector<std::string> getUserStatePath(const std::string &pk);

    REF_getter<bc_contract> getContract(const std::string &name);
    REF_getter<bc_contract> addContract(const std::string &name, const REF_getter<fee_calcer>& bca);

    REF_getter<bc_values> getValues();
    REF_getter<bc_values> checkValues();

    REF_getter<bc_epoch> getEpoch();



    REF_getter<bc_user> getUser(const std::string &pk);

    REF_getter<bc_user_state> getUserState(const std::string &pk);
    REF_getter<bc_user_state>   checkUserState(const std::string &pk);


    // std::vector<NODE_id> getNodesNames();
    std::vector<REF_getter<bc_node>> getAllNodes();


    REF_getter<bc_node> getNode(const NODE_id &name);
    REF_getter<bc_node> addNode(const NODE_id &name, const REF_getter<fee_calcer>& bc);



};
REF_getter<root_data> getRoot(IDatabase* db);

