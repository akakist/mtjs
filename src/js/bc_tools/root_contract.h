#pragma once
#include <string>
// #include <auto_mpz_t.h>
#include <nlohmann/json.hpp>
// #include "base62.h"
#include "blst_cp.h"
#include <stdint.h>
#include "IUtils.h"
#include "blake2bHasher.h"
#include "cellable.h"
#include "bls_io.h"
#include <fcntl.h>
#include "ioBuffer.h"
#include "bigint.h"
#include "base62.h"
#include "NODE_id.h"
#include "msg.h"
#include "hsh.h"

struct bc_contract:  public data_base
{
    bc_contract():data_base(hsh::bc_contract) {}
    std::string name;
    std::string owner;
    std::string src;
    REF_getter<bc_contract> copy() const
    {
        REF_getter<bc_contract> r(new bc_contract);
        r->name=name;
        r->owner=owner;
        r->src=src;
        return r;
    }
    void pack(outBuffer&b) const final
    {
        data_base::pack(b);
        b<<1;
        b<<name<<owner<<src;

    }
    void unpack(inBuffer&b) final
    {
        data_base::unpack(b);
        auto v=b.get_PN();
        b>>name>>owner>>src;

    }
    std::string dump() final
    {
        std::ostringstream o;
        o<<"Contract: "  << name << std::endl;
        o<< "Owner: " << base62::encode(owner) << std::endl;
        o<< "Src: " << src << std::endl;

        return o.str();
    }
};
struct bc_user: public data_base
{
    bc_user(): data_base(hsh::bc_user) {
    }
    std::string pkbin_еd;
    std::map<NODE_id /*nodeName*/, BigInt /*stake*/> my_stakes;
    std::set<std::string> nodes;
    std::set<std::string> contracts;

    REF_getter<bc_user> copy() const 
    {
        REF_getter<bc_user> r(new bc_user);
        r->pkbin_еd=pkbin_еd;
        r->my_stakes=my_stakes;
        r->nodes=nodes;
        r->contracts=contracts;
        return r;
    }
    void pack(outBuffer& o) const final
    {
        data_base::pack(o);
        o<<1;
        o<<pkbin_еd<<my_stakes<<nodes<<contracts;
    }
    void unpack(inBuffer& o) final
    {
        data_base::unpack(o);
        auto v=o.get_PN();
        o>>pkbin_еd>>my_stakes>>nodes>>contracts;
    }
    std::string dump() final
    {
        std::ostringstream o;
        o<<"PK: "  << base62::encode(pkbin_еd) << std::endl;
        o<< "Stakes: ";
        for(auto& z: my_stakes)
        {
            o<< z.first.container <<"->"<< z.second.toString();
        }
        o<< std::endl;

        o<< "Nodes: ";
        for(auto& z: nodes)
        {
            o<< z <<" ";
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
    bc_user_state(): data_base(hsh::bc_user_state) {
        nonce=0;
        balance=0;
    }
    BigInt balance;
    BigInt nonce;

    REF_getter<bc_user_state> copy() const
    {
        REF_getter<bc_user_state> r(new bc_user_state);
        r->balance=balance;
        r->nonce=nonce;
        return r;
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
        std::ostringstream o;
        o<< "Balance: " << balance.toString() << std::endl;
        o<< "Nonce: " << nonce.toString() <<std::endl;
        o<< std::endl;
        return o.str();
    }

};

struct bc_node: public data_base
{

    bc_node():data_base(hsh::bc_node) {
        total_stake=0;
    }
    NODE_id name;
    std::string owner_ed_pk;
    blst_cpp::PublicKey bls_pk;
    std::string ed_pk;
    std::string ip;
    std::map<std::string /*user*/, BigInt> stakes;
    BigInt total_stake;
    REF_getter<bc_node> copy() const
    {
        REF_getter<bc_node> r(new bc_node);
        r->name=name;
        r->owner_ed_pk=owner_ed_pk;
        r->bls_pk=bls_pk;
        r->ed_pk=ed_pk;
        r->ip=ip;
        r->stakes=stakes;
        r->total_stake=total_stake;
        return r;
    }
    void pack(outBuffer& o)  const final
    {
        data_base::pack(o);
        o<<1;
        o<<name<<owner_ed_pk<<bls_pk<<ed_pk<<ip<<stakes<<total_stake;
    }
    void unpack(inBuffer& o) final
    {
        data_base::unpack(o);
        auto v=o.get_PN();

        o>>name>>owner_ed_pk>>bls_pk>>ed_pk>>ip>>stakes>>total_stake;
    }
    std::string dump() final
    {
        std::ostringstream o;
        o<< "Node: "<< name.container << std::endl;
        o<< "Owner: "<< base62::encode(owner_ed_pk) << std::endl;
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

    bc_values(): data_base(hsh::bc_values) {
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
    std::set<std::string> emitters;
    REF_getter<bc_values> copy() const
    {
        REF_getter<bc_values> r(new bc_values);
        r->fees=fees;
        r->total_staked=total_staked;
        r->emitters=emitters;
        return r;
    }   
    void pack(outBuffer& o) const final
    {
        // cost.pack(o);
        o<<1;
        o<<fees<<total_staked<<emitters;
    }
    void unpack(inBuffer& o) final
    {
        // cost.unpack(o);
        auto v=o.get_PN();

        o>>fees>>total_staked>>emitters;
    }
    std::string dump() final
    {
        return "Values";
    }

};

struct bc_epoch: public data_base
{
    bc_epoch(): data_base(hsh::bc_epoch) {
        epoch=0;
    }
    BigInt epoch;
    REF_getter<bc_epoch> copy() const 
    {
        REF_getter<bc_epoch> r(new bc_epoch);
        r->epoch=epoch;
        return r;
    }   
    void pack(outBuffer& o) const final
    {
        // cost.pack(o);
        o<<1;
        o<<epoch;
    }
    void unpack(inBuffer& o) final
    {
        // cost.unpack(o);
        auto v=o.get_PN();

        o>>epoch;
    }
    std::string dump() final
    {
        return "Values";
    }

};

REF_getter<Cellable> getByPathOrCreate(REF_getter<Cellable> cur, const std::vector<std::string>& v, IDatabase* db, const REF_getter<fee_calcer>& bc);
REF_getter<Cellable> getByPathNoCreate(REF_getter<Cellable> cur, const std::vector<std::string>& v, IDatabase* db, const REF_getter<fee_calcer>& bc);


struct root_data: public Cellable
{
    REF_getter<IDatabase> db;
    root_data(IDatabase *db_): Cellable(nullptr,"r",NULL),db(db_)
    {
        accessed=true;
    }
    bool verify_lider_certificate(const REF_getter<MsgEvent::LeaderCertificate>& lc);

    std::vector<std::string> getContractPath(const std::string &name);
    std::vector<std::string> getNodePath(const std::string &name);
    std::vector<std::string> getUserPath(const std::string &pk);
    std::vector<std::string> getUserStatePath(const std::string &pk);

    REF_getter<const bc_contract> getContract(const std::string &name, const REF_getter<fee_calcer>& bc);
    void addContract(const std::string &name, const REF_getter<fee_calcer>& bca,const REF_getter<bc_contract> &c);
    void setContract(const std::string &name, const REF_getter<fee_calcer>& bca,const REF_getter<bc_contract> &c);

    REF_getter<bc_values> getValues(const REF_getter<fee_calcer>& bc);
    void setValues(const REF_getter<fee_calcer>& bc, const REF_getter<bc_values> &v);

    REF_getter<const bc_epoch> getEpoch(const REF_getter<fee_calcer>& bc);
    void setEpoch(const REF_getter<bc_epoch> &v);



    REF_getter<const bc_user> getUser(const std::string &pk, const REF_getter<fee_calcer>& bc);
    void setUser(const std::string& pk, const REF_getter<fee_calcer>& bc, const REF_getter<bc_user> &u);
    void addUser(const std::string &pk, const REF_getter<fee_calcer>& bc, const REF_getter<bc_user> &u);

    REF_getter<bc_user_state> getUserState(const std::string &pk, const REF_getter<fee_calcer>& bc);
    void addUserState(const std::string& pk, const REF_getter<fee_calcer>& bc, const REF_getter<bc_user_state> &u);
    void setUserState(const std::string& pk, const REF_getter<fee_calcer>& bc, const REF_getter<bc_user_state> &u);

    std::vector<NODE_id> getNodesNames( const REF_getter<fee_calcer>& bc);

    REF_getter<bc_node> getNode(const NODE_id &name, const REF_getter<fee_calcer>& bc);
    void addNode(const NODE_id &name, const REF_getter<fee_calcer>& bc,const REF_getter<bc_node> &n);
    void setNode(const NODE_id &name, const REF_getter<fee_calcer>& bc, const REF_getter<bc_node> &n);



};
REF_getter<root_data> getRoot(IDatabase* db);

