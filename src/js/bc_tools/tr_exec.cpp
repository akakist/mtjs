#include <fcntl.h>
#include <optional>
#include <xyjson.h>
#include <string>
#include "commonError.h"
#include "bigint.h"
#include "root_contract.h"
#include "tr_exec.h"
#include "ADDRESS_id.h"
#include "PK_id.h"

std::optional<std::string> TR::execute_mint(const yyjson::Value &params, t_params &t, 
        const ADDRESS_id &senderAddress, const REF_getter<fee_calcer> &by, const THASH_id &txid, int seqId, const EPOCH_id& epoch)
{
    auto v = t.root->getValues();
    auto it = v->emitters_bin.find(senderAddress);
    if (it == v->emitters_bin.end())
    {
        logErr2("insufficient_privileges");
        return "insufficient_privileges";
    }
    auto _amount = params / "amount";
    if (!_amount.isString())
    {
        logErr2("param string amount required");
        return "param string amount required";
    }
    BigInt amount;
    amount.from_string(_amount.toString());

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
    t.addCalcer(u.get(),by);

    t.fee[senderAddress] += v->getFee("mint");

    t.emit_command(txid, seqId,"mint",R"({"to":"%s","amount":"%s"})",
        base16::encode(senderAddress.addr).c_str(),
        amount.toString().c_str());

    return std::nullopt;
}
std::optional<std::string> TR::execute_transfer(const yyjson::Value &params, t_params &t, 
    const ADDRESS_id &senderAddress, const REF_getter<fee_calcer> &by, const THASH_id &txid, int seqId, const EPOCH_id& epoch)
{
    auto v = t.root->getValues();

    auto _a= params / "amount";
    if(!_a.isString())
    {
        return "param amount must be string";
    }

    BigInt amount;
    amount.from_string(_a.toString());

    auto _to=params / "to";
    if(!_to.isString())
        return "param to must be string";
    ADDRESS_id to_addr;

    to_addr.addr=base16::decode(_to.toString());
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
    t.addCalcer(u.get(),by);
    t.addCalcer(to.get(),by);


    t.fee[senderAddress] += fee;

    t.emit_command(txid, seqId, "transfer", R"({"from":"%s","to":"%s","amount":"%s"})", 
        base16::encode(senderAddress.addr).c_str(), 
        base16::encode(to_addr.addr).c_str(), 
        amount.toString().c_str()
        );

    return std::nullopt;
}
std::optional<std::string> TR::execute_node_update(const yyjson::Value &params, t_params &t, 
    const ADDRESS_id &senderAddress, const REF_getter<fee_calcer> &by, const THASH_id &txid, int seqId, const EPOCH_id& epoch)
{
    // if(senderAddress!=)
    auto v = t.root->getValues();
    NODE_id name;
    auto _name=params / "name";

    if(!_name.isString())
    {
        return "param name must be string";
    }
    name.container = _name.toString();


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



    auto ip=params / "ip";
    if (ip.isString())
    {
        nn->set_ip(ip.toString());
        // t.logMsg(txid, seqId, "ip changed");
        t.emit_command(txid, seqId, "node_change_ip",R"({"node":"%s","ip":"%s"})",name.container.c_str(),ip.toString().c_str());
        
    }


    nn->setDirty(epoch);
    us->setDirty(epoch);
    t.addCalcer(nn.get(),by);
    t.addCalcer(us.get(),by);


    t.fee[senderAddress] += fee;

    // t.logMsg(txid, seqId, "node %s updated", name.container.c_str());

    return std::nullopt;
}

std::optional<std::string> TR::execute_node_create(const yyjson::Value &params, t_params &t, 
    const ADDRESS_id &senderAddress, const REF_getter<fee_calcer> &by, const THASH_id &txid, int seqId, const EPOCH_id& epoch)
{
    auto v = t.root->getValues();
    NODE_id name;
    auto _name=params / "name";
    if(!_name.isString())
    {
        return "no string param name";
    }
    // if (!params.contains("name"))
    //     return "param name required";
    name.container = _name.toString();
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

    auto ip=params / "ip";
    auto pk_ed=params / "pk_ed";
    auto pk_bls=params / "pk_bls";
    if(!ip.isString())
        return "string param ip required";
    if(!pk_ed.isString())
        return "string param pk_ed required";
    if(!pk_bls.isString())
        return "string param pk_bls required";

        blst_cpp::PublicKey bls;
        bls.deserializeHexStr(pk_bls.toString());
    n->init(name, 
        senderAddress, 
        bls, 
        base16::decode(pk_ed.toString()), ip.toString());
    // n->name_ = name;
    // n->ip = ip.toString();
    // n->ed_pk = base16::decode(pk_ed.toString());
    // n->bls_pk.deserialize(base16::decode(pk_bls.toString()));
    // n->owner_ed_pk = senderAddress;
    n->setDirty(epoch);
    // u->setDirty();
    us->setDirty(epoch);
    t.addCalcer(n.get(),by);
    t.addCalcer(us.get(),by);



    t.fee[senderAddress] += fee;

    // t.logMsg(txid, seqId, "node %s registered", name.container.c_str());
    t.emit_command(txid, seqId, "node_create",R"({"node":"%s","ip":"%s"})",name.container.c_str(),ip.toString().c_str());

    return std::nullopt;
}
std::optional<std::string> TR::execute_node_stake(const yyjson::Value &params, t_params & t,
    const ADDRESS_id& senderAddress, const REF_getter<fee_calcer>& by, const THASH_id& txid, int seqId, const EPOCH_id& epoch)
{
    auto v = t.root->getValues();
    auto _amount = params / "amount";
    if (!_amount.isString())
    {
        return "param string amount required";
    }
    BigInt amount;
    amount.from_string(_amount.toString());
    auto _node = params / "node";
    if (!_node.isString())
    {
        return "param string node required";
    }
    NODE_id node;
    node.container = _node.toString();

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

    t.fee[senderAddress] += fee;

    // t.logMsg(txid, seqId, "node %s staked on amount %s", node.container.c_str(), amount.toString().c_str());
    t.emit_command(txid, seqId, "node_stake",R"({"node":"%s","stake":"%s","from":"%s"})",
        node.container.c_str(),
        amount.toString().c_str(),
        base16::encode(senderAddress.addr).c_str()
    );
    n->setDirty(epoch);
    us->setDirty(epoch);
    t.addCalcer(n.get(),by);
    t.addCalcer(us.get(),by);

    return std::nullopt;
}
std::optional<std::string> TR::execute_unstake_node(const yyjson::Value &params, t_params & t,
    const ADDRESS_id& senderAddress, const REF_getter<fee_calcer>& by, const THASH_id& txid, int seqId, const EPOCH_id& epoch)
{
    auto v = t.root->getValues();
    auto _amount = params / "amount";
    if (!_amount.isString())
    {
        return "param string amount required";
    }
    BigInt amount;
    amount.from_string(_amount.toString());
    auto _node = params / "node";
    if (!_node.isString())    {
        return "param string node required";
    }
    NODE_id node;
    node.container = _node.toString();
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
    t.addCalcer(v.get(),by);
    t.addCalcer(n.get(),by);
    t.addCalcer(u.get(),by);

    t.fee[senderAddress] += fee;

    // t.logMsg(txid, seqId, "node %s unstaked on amount %s", node.container.c_str(), amount.toString().c_str());
    t.emit_command(txid, seqId, "node_unstake",R"({"node":"%s","from":"%s","amount":"%s"})",
        node.container.c_str(),
        base16::encode(senderAddress.addr).c_str(),
        amount.toString().c_str()
    );

    return std::nullopt;
}

std::optional<std::string> TR::execute_node_enable(const yyjson::Value &params, t_params & t,
    const ADDRESS_id& senderAddress, const REF_getter<fee_calcer>& by, const THASH_id& txid, int seqId, const EPOCH_id& epoch)
{
    auto v = t.root->getValues();
    auto _node = params / "node";
    if (!_node.isString())
    {
        return "param string node required";
    }
    NODE_id node;
    node.container = _node.toString();

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

    t.fee[senderAddress] += fee;

    // t.logMsg(txid, seqId, "node %s enabled", node.container.c_str());
    t.emit_command(txid, seqId, "node_enable",R"({"node":"%s"})", node.container.c_str());

    return std::nullopt;
}




std::optional<std::string> TR::execute_contract_deploy(const yyjson::Value &params, t_params &t, 
    const ADDRESS_id &senderAddress, const REF_getter<fee_calcer> &by, const THASH_id &txid, int seqId, const EPOCH_id& epoch)
{
    auto v = t.root->getValues();
    std::string name;
    auto _name=params / "name";
    if(!_name.isString())
    {
        return "no string param name";
    }
    name = _name.toString();
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

    auto nn = t.root->getContract(name);
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


    auto n = t.root->addContract(name, by,epoch);

    auto src=params / "src";
    if(!src.isString())
        return "string param src required";

    {
        M_LOCK(n->parent->mx);
        n->src=src.toString();
        n->owner=senderAddress;
    }

    n->setDirty(epoch);
    us->setDirty(epoch);
    t.addCalcer(n.get(),by);
    t.addCalcer(us.get(),by);

    t.fee[senderAddress] += fee;

    t.emit_command(txid, seqId, "contract_deploy",R"({"name":"%s","owner":"%s"})",name.c_str(),base16::encode(senderAddress.addr).c_str());

    return std::nullopt;
}
std::optional<std::string> TR::execute_contract_update(const yyjson::Value &params, t_params &t, 
    const ADDRESS_id &senderAddress, const REF_getter<fee_calcer> &by, const THASH_id &txid, int seqId, const EPOCH_id& epoch)
{
    auto v = t.root->getValues();
    std::string name;
    auto _name=params / "name";
    if(!_name.isString())
    {
        return "no string param name";
    }
    name = _name.toString();

    auto n = t.root->getContract(name);
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

    auto src=params / "src";
    if(!src.isString())
        return "string param src required";

    {
        M_LOCK(n->parent->mx);
        n->src=src.toString();
    }

    n->setDirty(epoch);
    us->setDirty(epoch);
    t.addCalcer(n.get(),by);
    t.addCalcer(us.get(),by);

    t.fee[senderAddress] += fee;

    t.emit_command(txid, seqId, "contract_update",R"({"name":"%s","owner":"%s"})",name.c_str(),base16::encode(senderAddress.addr).c_str());

    return std::nullopt;
}
