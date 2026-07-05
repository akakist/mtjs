#include <fcntl.h>
#include <optional>
#include <yyjson.h>
#include <string>
#include "commonError.h"
#include "bigint.h"
#include "root_contract.h"
#include "tr_exec.h"
#include "ADDRESS_id.h"
#include "PK_id.h"
std::optional<std::string> yy_get_string(yyjson_val *params, const char *key, std::string& out)
{
    auto _name=yyjson_obj_get(params,key);
    if(!_name)
        return "param '"+(std::string)key+"' must be specified";

    if(!yyjson_is_str(_name))
        return "'"+(std::string)key+"' must be string";
    out= yyjson_get_str(_name);
    return std::nullopt;

}
std::optional<std::string> yy_get_bn(yyjson_val *params, const char *key, BigInt& out)
{
    auto _name=yyjson_obj_get(params,key);
    if(!_name)
        return "param '"+(std::string)key+"' must be specified";

    if(yyjson_is_str(_name))
        out.from_string(yyjson_get_str(_name));
    else if(yyjson_is_num(_name))
        out=yyjson_get_uint(_name);
    else return "'"+(std::string)key+"' must be string or num";

    return std::nullopt;

}

std::optional<std::string> TR::execute_mint(yyjson_val *params, b_params &b, t_params &t,
        const ADDRESS_id &senderAddress, const REF_getter<fee_calcer> &by, const THASH_id &txid, int seqId, const EPOCH_id& epoch)
{
    auto v = t.root->getValues();
    auto it = v->emitters_bin.find(senderAddress);
    if (it == v->emitters_bin.end())
    {
        logErr2("insufficient_privileges");
        return "insufficient_privileges";
    }
    BigInt amount=0;
    auto err=yy_get_bn(params,"amount",amount);
    if(err) return err;


    auto u = t.root->getUser(senderAddress);
    if (!u.valid())
    {
        return "mint: sender not found";
    }
    {
        M_LOCK(u->parent->mx);
        u->balance+=amount;
    }
    // u->addBalance(amount);
    u->setDirty(epoch);
    b.addCalcer(u.get(),by);

    b.fee[senderAddress] += v->getFee("mint");

    b.emit_command(txid, seqId,"mint",R"({"to":"%s","amount":"%s"})",
        base16::encode(senderAddress.addr).c_str(),
        amount.toString().c_str());

    return std::nullopt;
}
std::optional<std::string> TR::execute_transfer(yyjson_val *params, b_params &b, t_params &t,
    const ADDRESS_id &senderAddress, const REF_getter<fee_calcer> &by, const THASH_id &txid, int seqId, const EPOCH_id& epoch)
{
    auto v = t.root->getValues();

    BigInt amount=0;
    auto err=yy_get_bn(params,"amount",amount);
    if(err) return err;

    std::string to_;
    err=yy_get_string(params,"to",to_);
    if(err) return err;

    ADDRESS_id to_addr;
    to_addr.addr=base16::decode(to_);
    if(to_addr.addr.size()!=senderAddress.addr.size())
        return "param to has invalid size";

    auto u = t.root->getUser(senderAddress);
    if (!u.valid())
    {
        return "sender userstate invalid";
    }
    // auto to_addr = params["to"].get<std::string>();
    if (to_addr == senderAddress)
    {
        return "cannot transfer to self";
    }
    if (to_addr.addr.size() != senderAddress.addr.size())
    {
        return "invalid destination address";
    }
    auto to = t.root->getUser(to_addr);
    if (!to.valid())
    {
        return "destination user not found";
    }
    auto fee=v->getFee("transfer");
    {
        M_LOCK(u->parent->mx);
        if(u->balance < fee + amount)
            return "Not enough funds";
        u->balance-=amount;
    }
    // if (u->getBalance() < fee + amount)
    // {
    //     return "Not enough funds";
    // }
    // u->subBalance(amount);
    {
        M_LOCK(to->parent->mx);
        to->balance+=amount;
    }
    // to->addBalance(amount);
    u->setDirty(epoch);
    to->setDirty(epoch);
    b.addCalcer(u.get(),by);
    b.addCalcer(to.get(),by);


    b.fee[senderAddress] += fee;

    b.emit_command(txid, seqId, "transfer", R"({"from":"%s","to":"%s","amount":"%s"})", 
        base16::encode(senderAddress.addr).c_str(), 
        base16::encode(to_addr.addr).c_str(), 
        amount.toString().c_str()
        );

    return std::nullopt;
}
std::optional<std::string> TR::execute_node_update(yyjson_val *params, b_params &b, t_params &t,
    const ADDRESS_id &senderAddress, const REF_getter<fee_calcer> &by, const THASH_id &txid, int seqId, const EPOCH_id& epoch)
{
    // if(senderAddress!=)
    auto v = t.root->getValues();
    NODE_id name;
    auto err=yy_get_string(params,"name",name.container);
    if(err) return err;



    auto nn = t.root->getNode(name);
    if (!nn.valid())
        return "Node not found";
    if(nn->get_owner()!=senderAddress)
    {
        return "only node owner can update node info";
    }
    auto us = t.root->getUser(senderAddress);
    if (!us.valid())
        return "if(!us.valid())";
    auto fee=v->getFee("node_update");
    {
        M_LOCK(us->parent->mx);
        if(us->balance < fee)
        return "Not enough funds";
    }

    std::string ip;
    err=yy_get_string(params,"ip",ip);
    if(err) return err;

    {
        nn->set_ip(ip);
        // t.logMsg(txid, seqId, "ip changed");
        b.emit_command(txid, seqId, "node_change_ip",R"({"node":"%s","ip":"%s"})",name.container.c_str(),ip.c_str());
        
    }


    nn->setDirty(epoch);
    us->setDirty(epoch);
    b.addCalcer(nn.get(),by);
    b.addCalcer(us.get(),by);


    b.fee[senderAddress] += fee;

    // t.logMsg(txid, seqId, "node %s updated", name.container.c_str());

    return std::nullopt;
}

std::optional<std::string> TR::execute_node_create(yyjson_val *params, b_params &b, t_params &t,
    const ADDRESS_id &senderAddress, const REF_getter<fee_calcer> &by, const THASH_id &txid, int seqId, const EPOCH_id& epoch)
{
    auto v = t.root->getValues();
    NODE_id name;
    auto err=yy_get_string(params,"name",name.container);
    if(err) return err;
    for (auto &z : name.container)
    {
        if (!isalnum(z))
        {
            return "allowed only isalnum symbols";
        }
        if (isalpha(z) && isupper(z))
        {
            return "allowed only lowercase symbols";
        }
    }

    auto nn = t.root->getNode(name);
    if (nn.valid())
        return "Node already registered with name";

    auto us = t.root->getUser(senderAddress);
    if (!us.valid())
        return "if(!us.valid())";
    auto fee=v->getFee("node_create");
    {
        M_LOCK(us->parent->mx);
        if (us->balance < fee)
            return "Not enough funds";
    }


    auto n = t.root->addNode(name, by,epoch);

    std::string ip,pk_ed,pk_bls;
    err=yy_get_string(params,"ip",ip);
    if(err) return err;
    err=yy_get_string(params,"pk_ed",pk_ed);
    if(err) return err;
    err=yy_get_string(params,"pk_bls",pk_bls);
    if(err) return err;
    // auto ip=params / "ip";
    // auto pk_ed=params / "pk_ed";
    // auto pk_bls=params / "pk_bls";
    // if(!ip.isString())
    //     return "string param ip required";
    // if(!pk_ed.isString())
    //     return "string param pk_ed required";
    // if(!pk_bls.isString())
    //     return "string param pk_bls required";

        blst_cpp::PublicKey bls;
        bls.deserializeHexStr(pk_bls);
    n->init(name, 
        senderAddress, 
        bls, 
        base16::decode(pk_ed), ip);
    // n->name_ = name;
    // n->ip = ip.toString();
    // n->ed_pk = base16::decode(pk_ed.toString());
    // n->bls_pk.deserialize(base16::decode(pk_bls.toString()));
    // n->owner_ed_pk = senderAddress;
    n->setDirty(epoch);
    // u->setDirty();
    us->setDirty(epoch);
    b.addCalcer(n.get(),by);
    b.addCalcer(us.get(),by);



    b.fee[senderAddress] += fee;

    // t.logMsg(txid, seqId, "node %s registered", name.container.c_str());
    b.emit_command(txid, seqId, "node_create",R"({"node":"%s","ip":"%s"})",name.container.c_str(),ip.c_str());

    return std::nullopt;
}
std::optional<std::string> TR::execute_node_stake(yyjson_val *params, b_params & b,t_params &t,
    const ADDRESS_id& senderAddress, const REF_getter<fee_calcer>& by, const THASH_id& txid, int seqId, const EPOCH_id& epoch)
{
    auto v = t.root->getValues();

    BigInt amount=0;
    auto err=yy_get_bn(params,"amount",amount);
    if(err) return err;

    NODE_id node;
    err=yy_get_string(params,"node",node.container);
    if(err) return err;


    auto us = t.root->getUser(senderAddress);
    if (!us.valid())
        return "if(!us.valid())";
    auto fee=v->getFee("node_stake");

    auto n=t.root->getNode(node);
    if(!n.valid())
    {
        return "node not found";
    }
    // BigInt &nodeStake = n->get_stastakes[senderAddress];
    {
        M_LOCK(us->parent->mx);
        if (us->balance < amount + fee)
        {
            return "Insufficient funds";
        }

    // }

    // {
    //     M_LOCK(us->parent->mx);
        us->balance-=amount;
    }
    // us->subBalance(amount);
    n->add_stake(senderAddress, amount);
    //  auto nodeStake = n->getStake(senderAddress);
    // nodeStake += amount;
    // n->total_stake += amount;
    // v->total_staked += amount;

    b.fee[senderAddress] += fee;

    // t.logMsg(txid, seqId, "node %s staked on amount %s", node.container.c_str(), amount.toString().c_str());
    b.emit_command(txid, seqId, "node_stake",R"({"node":"%s","stake":"%s","from":"%s"})",
        node.container.c_str(),
        amount.toString().c_str(),
        base16::encode(senderAddress.addr).c_str()
    );
    n->setDirty(epoch);
    us->setDirty(epoch);
    b.addCalcer(n.get(),by);
    b.addCalcer(us.get(),by);

    return std::nullopt;
}
std::optional<std::string> TR::execute_unstake_node(yyjson_val *params, b_params & b,t_params &t,
    const ADDRESS_id& senderAddress, const REF_getter<fee_calcer>& by, const THASH_id& txid, int seqId, const EPOCH_id& epoch)
{
    auto v = t.root->getValues();
    BigInt amount=0;
    auto err=yy_get_bn(params,"amount",amount);
    if(err) return err;
    NODE_id node;
    err=yy_get_string(params,"node",node.container);
    if(err) return err;
    auto n = t.root->getNode(node);
    if (!n.valid())
    {
        return "nodes not registered";
    }
    auto stake=n->get_user_stake(senderAddress);
    // auto nodeStakeIt = n->stakes.find(senderAddress);
    if (stake == 0)
    {
        return "no_stake_found";
    }
    // auto &nodeStake = nodeStakeIt->second;
    if (stake < amount)
    {
        return "insufficient stake in node";
    }

    auto u = t.root->getUser(senderAddress);

    if (!u.valid())
        return "FATAL:  dst addr not found";
    auto fee=v->getFee("node_unstake");
    {

    }
    {
        M_LOCK(u->parent->mx);
        if(u->balance < fee)
        {
            return "Insufficient funds to unstake";
        }
        u->balance+=amount;

    }

    n->sub_stake(senderAddress, amount);
    // nodeStake -= amount;

    // n->total_stake -= amount;


    // v->total_staked -= amount;

    v->setDirty(epoch);
    n->setDirty(epoch);
    u->setDirty(epoch);
    b.addCalcer(v.get(),by);
    b.addCalcer(n.get(),by);
    b.addCalcer(u.get(),by);

    b.fee[senderAddress] += fee;

    // t.logMsg(txid, seqId, "node %s unstaked on amount %s", node.container.c_str(), amount.toString().c_str());
    b.emit_command(txid, seqId, "node_unstake",R"({"node":"%s","from":"%s","amount":"%s"})",
        node.container.c_str(),
        base16::encode(senderAddress.addr).c_str(),
        amount.toString().c_str()
    );

    return std::nullopt;
}

std::optional<std::string> TR::execute_node_enable(yyjson_val *params, b_params & b,t_params &t,
    const ADDRESS_id& senderAddress, const REF_getter<fee_calcer>& by, const THASH_id& txid, int seqId, const EPOCH_id& epoch)
{
    auto v = t.root->getValues();
    NODE_id node;
    auto err=yy_get_string(params,"node",node.container);
    if(err)return err;

    auto n = t.root->getNode(node);
    if (!n.valid())
    {
        return "node not found";
    }
    if (n->get_owner() != senderAddress)
    {
        return "only node owner can enable node owner "+base16::encode(n->get_owner().addr)+" " + base16::encode(senderAddress.addr);
    }
    auto fee=v->getFee("node_enable");
    auto us = t.root->getUser(senderAddress);
    if (!us.valid())
        return "if(!us.valid())";
    {
        M_LOCK(us->parent->mx);     
        if (us->balance < fee)
        {
            return "Insufficient funds";
        }
    }
    n->reset_missed_rounds();
    n->setDirty(epoch);

    b.fee[senderAddress] += fee;

    // t.logMsg(txid, seqId, "node %s enabled", node.container.c_str());
    b.emit_command(txid, seqId, "node_enable",R"({"node":"%s"})", node.container.c_str());

    return std::nullopt;
}




std::optional<std::string> TR::execute_contract_deploy(yyjson_val *params, b_params &b, t_params &t,
    const ADDRESS_id &senderAddress, const REF_getter<fee_calcer> &by, const THASH_id &txid, int seqId, const EPOCH_id& epoch)
{
    auto v = t.root->getValues();
    std::string name;
    auto err=yy_get_string(params,"name",name);
    if(err) return err;
    for (auto &z : name)
    {
        if (!isalnum(z))
        {
            return "allowed only isalnum symbols";
        }
        if (isalpha(z) && isupper(z))
        {
            return "allowed only lowercase symbols";
        }
    }
    CONTRACT_id cn;
    cn.container=name;
    auto nn = t.root->getContract(cn);
    if (nn.valid())
        return "Contract already registered with name";

    auto us = t.root->getUser(senderAddress);
    if (!us.valid())
        return "if(!us.valid())";
    auto fee=v->getFee("contract_deploy");
    {
        M_LOCK(us->parent->mx);
        if (us->balance < fee)
            return "Not enough funds";
    }


    auto n = t.root->addContract(cn, by,epoch);

    std::string src;
     err=yy_get_string(params,"src",src);
    if(err)return err;

    {
        M_LOCK(n->parent->mx);
        n->src=src;
        n->owner=senderAddress;
    }

    n->setDirty(epoch);
    us->setDirty(epoch);
    b.addCalcer(n.get(),by);
    b.addCalcer(us.get(),by);

    b.fee[senderAddress] += fee;

    b.emit_command(txid, seqId, "contract_deploy",R"({"name":"%s","owner":"%s"})",name.c_str(),base16::encode(senderAddress.addr).c_str());

    return std::nullopt;
}
std::optional<std::string> TR::execute_contract_update(yyjson_val *params, b_params &b, t_params &t,
    const ADDRESS_id &senderAddress, const REF_getter<fee_calcer> &by, const THASH_id &txid, int seqId, const EPOCH_id& epoch)
{
    auto v = t.root->getValues();
    CONTRACT_id cn;
    auto err=yy_get_string(params,"name",cn.container);
    if(err)
        return err;

    auto n = t.root->getContract(cn);
    if (!n.valid())
        return "contract not exists";

    if(senderAddress!=n->owner)
    {
        return "sender is not contract owner";
    }
    auto us = t.root->getUser(senderAddress);
    if (!us.valid())
        return "if(!us.valid())";
    auto fee=v->getFee("contract_update");
    {
        M_LOCK(us->parent->mx);
        if (us->balance < fee)
            return "Not enough funds";
    }
    


    // auto n = t.root->addContract(name, by);
    std::string src;
    err=yy_get_string(params,"src",src);
    if(err) return err;

    {
        M_LOCK(n->parent->mx);
        n->src=src;
    }

    n->setDirty(epoch);
    us->setDirty(epoch);
    b.addCalcer(n.get(),by);
    b.addCalcer(us.get(),by);

    b.fee[senderAddress] += fee;

    b.emit_command(txid, seqId, "contract_update",R"({"name":"%s","owner":"%s"})",cn.container.c_str(),base16::encode(senderAddress.addr).c_str());

    return std::nullopt;
}
