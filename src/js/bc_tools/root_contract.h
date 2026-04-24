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
    bool frozen=false;
    std::string src;
    bool ready_for_sale=false;
    BigInt cost;
    // std::string contract_state; // js contract variable
    void pack(outBuffer&b) const final
    {
        data_base::pack(b);
        b<<1;
        b<<name<<owner<<frozen<<src<<ready_for_sale<<cost;

    }
    void unpack(inBuffer&b) final
    {
        data_base::unpack(b);
        auto v=b.get_PN();
        b>>name>>owner>>frozen>>src>>ready_for_sale>>cost;

    }
    std::string dump() final
    {
        std::ostringstream o;
        o<<"Contract: "  << name << std::endl;
        o<< "Owner: " << base62::encode(owner) << std::endl;
        o<< "Frozen: " << frozen <<std::endl;
        o<< "Src: " << src << std::endl;
        o<< "Ready for sale: " <<ready_for_sale << std::endl;
        o<< "Cost: " <<cost.toString() << std::endl;

        return o.str();
    }
};
struct bc_user: public data_base
{
    bc_user(): data_base(hsh::bc_user) {
        nonce=0;
        balance=0;
    }
    std::string pkbin;
    BigInt balance;
    BigInt nonce;
    std::map<NODE_id /*nodeName*/, BigInt /*stake*/> my_stakes;
    std::set<std::string> nodes;
    std::set<std::string> contracts;

    void pack(outBuffer& o) const final
    {
        data_base::pack(o);
        o<<1;
        o<<pkbin<<balance<<nonce<<my_stakes<<nodes<<contracts;
    }
    void unpack(inBuffer& o) final
    {
        data_base::unpack(o);
        auto v=o.get_PN();
        o>>pkbin>>balance>>nonce>>my_stakes>>nodes>>contracts;
    }
    std::string dump() final
    {
        std::ostringstream o;
        o<<"PK: "  << base62::encode(pkbin) << std::endl;
        o<< "Balance: " << balance.toString() << std::endl;
        o<< "Nonce: " << nonce.toString() <<std::endl;
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
    bool disabled_manual=false;
    bool disabled_offline=false;
    void pack(outBuffer& o)  const final
    {
        data_base::pack(o);
        o<<1;
        o<<name<<owner_ed_pk<<bls_pk<<ed_pk<<ip<<stakes<<total_stake<<disabled_manual<<disabled_offline;
    }
    void unpack(inBuffer& o) final
    {
        data_base::unpack(o);
        auto v=o.get_PN();

        o>>name>>owner_ed_pk>>bls_pk>>ed_pk>>ip>>stakes>>total_stake>>disabled_manual>>disabled_offline;
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
        epoch=0;
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
    BigInt epoch;
    void pack(outBuffer& o) const final
    {
        // cost.pack(o);
        o<<1;
        o<<fees<<total_staked<<emitters<<epoch;
    }
    void unpack(inBuffer& o) final
    {
        // cost.unpack(o);
        auto v=o.get_PN();

        o>>fees>>total_staked>>emitters>>epoch;
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
    bool verify_lider_certificate(const msg::leader_certificate& lc);

    std::vector<std::string> getContractPath(const std::string &name);
    std::vector<std::string> getNodePath(const std::string &name);
    REF_getter<bc_contract> getContract(const std::string &name, const REF_getter<fee_calcer>& bc);
    REF_getter<bc_contract> addContract(const std::string &name, const REF_getter<fee_calcer>& bca);
    REF_getter<bc_values> getValues(const REF_getter<fee_calcer>& bc);

    std::vector<std::string> getUserPath(const std::string &pk);
    REF_getter<bc_user> getUser(const std::string &pk, const REF_getter<fee_calcer>& bc);
    REF_getter<bc_user> addUser(const std::string &pk, const REF_getter<fee_calcer>& bc);
    std::vector<std::string> getNickPath(const std::string &pk);
    // REF_getter<bc_nick> getNick(const std::string &nick, const REF_getter<fee_calcer>& bc);
    // REF_getter<bc_nick> addNick(const std::string &nick, const REF_getter<fee_calcer>& bc);
    std::vector<NODE_id> getNodesNames( const REF_getter<fee_calcer>& bc)
    {

        std::vector<NODE_id>v;
        auto nodes=this->getLeafNoCreate("n",db.get(),bc);
        for(auto &z: nodes->children_hashes)
        {
            NODE_id n;
            n.container=z.first;
            v.push_back(n);
        }
        return v;
    }
    REF_getter<bc_node> addNode(const NODE_id &name, const REF_getter<fee_calcer>& bc)
    {
        MUTEX_INSPECTOR;

        std::vector<std::string> v;
        v.push_back("n");
        v.push_back(name.container);
        auto cc=getByPathOrCreate(this,v,db.get(),bc);

        if(cc->data.valid())
            throw CommonError("if(cc->data.valid())");
        if(cc->payload.size())
            throw CommonError("if(cc->payload.size())");

        REF_getter<bc_node> u=new bc_node;
        cc->data=u.get();
        cc->payload_ctor_idx=hsh::bc_node;
        return u;
    }
    REF_getter<bc_node> getNode(const NODE_id &name, const REF_getter<fee_calcer>& bc)
    {
        MUTEX_INSPECTOR;
        std::vector<std::string> v;
        v.push_back("n");
        v.push_back(name.container);

        auto cc=getByPathNoCreate(this,v,db.get(),bc);
        if(!cc.valid())
            return NULL;
        if(cc->data.valid())
            return dynamic_cast<bc_node*>(cc->data.get());
        else throw CommonError("if(cc->data.valid())");

    }


};

inline REF_getter<root_data> getRoot(IDatabase* db)
{
    MUTEX_INSPECTOR;
    // logErr2("if(!root.valid())");

    REF_getter<root_data> r=new root_data(db);
    r->accessed=true;
    // std::string r;
    THASH_id root_hash;
    std::string root_cell;
    int err1=db->get_cell("#root_hash#",&root_hash.container);
    if(!err1)
    {
        int err=db->get_cell("#root#",&root_cell);
        if(!err)
        {
            if(blake2b_hash(root_cell)!=root_hash)
            {
                logErr2("if(blake2b_hash(root_cell)!=root_hash)");

            }
            else
            {
                MUTEX_INSPECTOR;
                inBuffer in(root_cell);
                r->unpack(in);
            }
        }
    }
    else logErr2("cannot read #root_hash#");


    // int err=db->get_cell("r",&r);
    // if(err)
    // {
    //     logErr2("error read db %s",_DMI().c_str());
    // }
    // else
    // {
    //     inBuffer in(r);
    //     root->unpack(in);
    // }
    return r;
}
