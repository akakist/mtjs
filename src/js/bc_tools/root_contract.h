#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include "blst_cp.h"
#include <stdint.h>
#include "IUtils.h"
#include "blake2bHasher.h"
#include "cellable.h"
#include <fcntl.h>
#include "ioBuffer.h"
#include "bigint.h"
#include "base62.h"
#include "NODE_id.h"
#include "msg.h"
#include "hsh.h"
#include "md/md_LeaderCertificate.h"

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
        o<< "Owner: " << base62::encode(owner) << std::endl;
        o<< "Src: " << src << std::endl;

        return o.str();
    }
};
struct bc_user: public data_base
{

    bc_user(Cellable* p): data_base(hsh::bc_user,p) {
    }
    std::string pkhex_еd;
    std::map<NODE_id /*nodeName*/, BigInt /*stake*/> my_stakes;
    std::set<NODE_id> nodes;
    std::set<std::string> contracts;

    void pack(outBuffer& o) const final
    {
        data_base::pack(o);
        o<<1;
        o<<pkhex_еd<<my_stakes<<nodes<<contracts;
    }
    void unpack(inBuffer& o) final
    {
        data_base::unpack(o);
        auto v=o.get_PN();
        o>>pkhex_еd>>my_stakes>>nodes>>contracts;
    }
    std::string dump() final
    {
        std::ostringstream o;
        o<<"PK: "  << pkhex_еd << std::endl;
        o<< "Stakes: ";
        for(auto& z: my_stakes)
        {
            o<< z.first.container <<"->"<< z.second.toString();
        }
        o<< std::endl;

        o<< "Nodes: ";
        for(auto& z: nodes)
        {
            o<< z.container <<" ";
        }
        o<< std::endl;


        o<< "Contracts: ";
        for(auto& z: contracts)
        {
            o<< z <<" ";
        }
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
    BigInt balance;
    BigInt nonce;

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
    NODE_id name_;
    std::string owner_ed_pkhex;
    blst_cpp::PublicKey bls_pk;
    std::string ed_pk;
    std::string ip;
    std::map<std::string /*user*/, BigInt> stakes;
    BigInt total_stake;
    void pack(outBuffer& o)  const final
    {
        data_base::pack(o);
        o<<1;
        o<<name_<<owner_ed_pkhex<<bls_pk<<ed_pk<<ip<<stakes<<total_stake;
    }
    void unpack(inBuffer& o) final
    {
        data_base::unpack(o);
        auto v=o.get_PN();

        o>>name_>>owner_ed_pkhex>>bls_pk>>ed_pk>>ip>>stakes>>total_stake;
    }
    std::string dump() final
    {
        std::ostringstream o;
        o<< "Node: "<< name_.container << std::endl;
        o<< "Owner: "<< owner_ed_pkhex << std::endl;
        o<< "bls_pk: "<< base62::encode(bls_pk.serialize()) << std::endl;
        o<< "ed_pk: "<< base62::encode(ed_pk) << std::endl;
        o<< "ip:port: "<< ip << std::endl;
        o<< "stake: "<< total_stake.toString() << std::endl;
        o<< "stakers: "<< std::endl;
        for(auto &z: stakes)
        {
            o<< "\t"<< base62::encode(z.first) << " -> " << z.second.toString() << std::endl;
        }
        return o.str();
    }

};

struct bc_values: public data_base
{

    enum __fee_types
    {
        contract_deploy,
        contract_transfer,
        node_create,
        node_enable_from_manual,
        node_enable_from_offline,
        nick_create,
        nick_transfer,
        cell_in_contract_create,
        freeze_contract,
        mint,
        unstake,
        stake,
        transfer,
        register_nick,

        FEE_TYPE_END
    };

    bc_values(Cellable *p): data_base(hsh::bc_values,p) {
        fees.resize(FEE_TYPE_END);
        fees[contract_deploy]=BigInt(5000);
        fees[contract_transfer]=BigInt(1000);
        fees[node_create]=BigInt(20000);
        fees[node_enable_from_manual]=BigInt(5000);
        fees[node_enable_from_offline]=BigInt(10000);
        fees[nick_create]=BigInt(2000);
        fees[nick_transfer]=BigInt(1000);
        fees[cell_in_contract_create]=BigInt(500);
        fees[freeze_contract]=BigInt(3000);
        fees[mint]=BigInt(95);
        fees[unstake]=BigInt(200);
        fees[stake]=BigInt(200);
        fees[transfer]=BigInt(1000);
        fees[register_nick]=BigInt(5000);
    }
    std::vector<BigInt> fees;
    BigInt total_staked;
    std::set<std::string> emitters_hex;
    void pack(outBuffer& o) const final
    {
        // cost.pack(o);
        o<<1;
        o<<fees<<total_staked<< emitters_hex;
    }
    void unpack(inBuffer& o) final
    {
        // cost.unpack(o);
        auto v=o.get_PN();

        o>>fees>>total_staked>>emitters_hex;
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


    std::vector<NODE_id> getNodesNames();

    REF_getter<bc_node> getNode(const NODE_id &name);
    REF_getter<bc_node> addNode(const NODE_id &name, const REF_getter<fee_calcer>& bc);



};
REF_getter<root_data> getRoot(IDatabase* db);

