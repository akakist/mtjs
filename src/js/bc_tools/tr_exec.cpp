#include <fcntl.h>
#include <optional>
#include <yyjson.h>
#include <string>
#include "commonError.h"
// #include "bigint.h"
#include "root_contract.h"
#include "tr_exec.h"
#include "ADDRESS_id.h"
#include "PK_id.h"
#include "yyjson_to_quickjs.h"

std::optional<std::string> TR::execute_mint(yyjson_val *params, b_params &b, t_params &t,
          int seqId)
{
        MUTEX_INSPECTOR;

    auto v = b.db->getValuesNoCreate();
    auto it = v->emitters_bin.find(t.senderAddress);
    if (it == v->emitters_bin.end())
    {
        logErr2("insufficient_privileges");
        return "insufficient_privileges";
    }
    uint64_t amount=0;
    auto err=yy_get_uint64_t(params,"amount",amount);
    if(err) return err;


    auto u = t.getAddressState(t.senderAddress);
    if (!u.valid())
    {
        return "mint: sender not found";
    }
    {
        M_LOCK(u->parent->mx);
        u->balance+=amount;
    }
    u->setDirty(t.roll);

    t.gasUsed+=v->getGas("mint");

    b.emit_command(t.tx_id, seqId,"mint",R"({"to":"%s","amount":"%s"})",
        base16::encode(t.senderAddress.addr).c_str(),
         std::to_string(amount).c_str());

    return std::nullopt;
}
std::optional<std::string> TR::execute_transfer(yyjson_val *params, b_params &b, t_params &t,
     int seqId)
{
    MUTEX_INSPECTOR;
    auto v = b.db->getValuesNoCreate();

    uint64_t amount=0;
    auto err=yy_get_uint64_t(params,"amount",amount);
    if(err) return err;

    std::string to_;
    err=yy_get_string(params,"to",to_);
    if(err) return err;

    ADDRESS_id to_addr;
    to_addr.addr=base16::decode(to_);
    if(to_addr.addr.size()!=t.senderAddress.addr.size())
        return "param to has invalid size";

    auto u = t.getAddressState(t.senderAddress);
    if (!u.valid())
    {
        return "sender userstate invalid";
    }
    // auto to_addr = params["to"].get<std::string>();
    if (to_addr == t.senderAddress)
    {
        return "cannot transfer to self";
    }
    if (to_addr.addr.size() != t.senderAddress.addr.size())
    {
        return "invalid destination address";
    }
    auto to = t.getAddressState(to_addr);
    if (!to.valid())
    {
        return "destination user not found";
    }
    if(amount>t.value)
    return "amount>t.value";

    t.value-=amount;
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
    u->setDirty(t.roll);
    to->setDirty(t.roll);
    t.gasUsed+=v->getGas("transfer");



    b.emit_command(t.tx_id, seqId, "transfer", R"({"from":"%s","to":"%s","amount":"%s"})", 
        base16::encode(t.senderAddress.addr).c_str(), 
        base16::encode(to_addr.addr).c_str(), 
        std::to_string(amount).c_str()
        );

    return std::nullopt;
}
std::optional<std::string> TR::execute_node_update(yyjson_val *params, b_params &b, t_params &t,
    int seqId)
{
    MUTEX_INSPECTOR;
    // if(senderAddress!=)
    auto v = b.db->getValuesNoCreate();
    NODE_id name;
    auto err=yy_get_string(params,"name",name.container);
    if(err) return err;



    auto nn = t.getNode(name);
    if (!nn.valid())
        return "Node not found";
    if(nn->get_owner()!=t.senderAddress)
    {
        return "only node owner can update node info";
    }
    auto us = t.getAddressState(t.senderAddress);
    if (!us.valid())
        return "if(!us.valid())";

    std::string ip;
    err=yy_get_string(params,"ip",ip);
    if(err) return err;

    {
        nn->set_ip(ip);
        // t.logMsg(txid, seqId, "ip changed");
        b.emit_command(t.tx_id, seqId, "node_change_ip",R"({"node":"%s","ip":"%s"})",name.container.c_str(),ip.c_str());
        
    }


    nn->setDirty(t.roll);
    us->setDirty(t.roll);


    t.gasUsed+=v->getGas("node_update");


    return std::nullopt;
}

std::optional<std::string> TR::execute_node_create(yyjson_val *params, b_params &b, t_params &t,
     int seqId)
{
    MUTEX_INSPECTOR;
    auto v = b.db->getValuesNoCreate();
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

    auto nn = t.getNode(name);
    if (nn.valid())
        return "Node already registered with name";

    auto us = t.getAddressState(t.senderAddress);
    if (!us.valid())
        return "if(!us.valid())";


    auto n = b.db->addNode(name, t.roll);

    std::string ip,pk_ed,pk_bls;
    err=yy_get_string(params,"ip",ip);
    if(err) return err;
    err=yy_get_string(params,"pk_ed",pk_ed);
    if(err) return err;
    err=yy_get_string(params,"pk_bls",pk_bls);
    if(err) return err;

        blst_cpp::PublicKey bls;
        bls.deserializeHexStr(pk_bls);
    n->init(name, 
        t.senderAddress, 
        bls, 
        base16::decode(pk_ed), ip);
    n->setDirty(t.roll);
    us->setDirty(t.roll);



    t.gasUsed+=v->getGas("node_create");

    // t.logMsg(txid, seqId, "node %s registered", name.container.c_str());
    b.emit_command(t.tx_id, seqId, "node_create",R"({"node":"%s","ip":"%s"})",name.container.c_str(),ip.c_str());

    return std::nullopt;
}
std::optional<std::string> TR::execute_node_stake(yyjson_val *params, b_params & b,t_params &t,
     int seqId)
{
    MUTEX_INSPECTOR;
    auto v = b.db->getValuesNoCreate();

    uint64_t amount=0;
    auto err=yy_get_uint64_t(params,"amount",amount);
    if(err) return err;

    NODE_id node;
    err=yy_get_string(params,"node",node.container);
    if(err) return err;


    auto us = t.getAddressState(t.senderAddress);
    if (!us.valid())
        return "if(!us.valid())";

    auto n=t.getNode(node);
    if(!n.valid())
    {
        return "node not found";
    }
    // BigInt &nodeStake = n->get_stastakes[senderAddress];
    if(t.value<amount)
    {
        return "Insufficient funds";
    }
    t.value-=amount;
    n->add_stake(t.senderAddress, amount);

    t.gasUsed += v->getGas("node_stake");

    b.emit_command(t.tx_id, seqId, "node_stake",R"({"node":"%s","stake":"%s","from":"%s"})",
        node.container.c_str(),
        std::to_string(amount).c_str(),
        base16::encode(t.senderAddress.addr).c_str()
    );
    n->setDirty(t.roll);
    us->setDirty(t.roll);

    return std::nullopt;
}
std::optional<std::string> TR::execute_unstake_node(yyjson_val *params, b_params & b,t_params &t,
     int seqId)
{
    MUTEX_INSPECTOR;
    auto v = b.db->getValuesNoCreate();
    uint64_t amount=0;
    auto err=yy_get_uint64_t(params,"amount",amount);
    if(err) return err;
    NODE_id node;
    err=yy_get_string(params,"node",node.container);
    if(err) return err;
    auto n = t.getNode(node);
    if (!n.valid())
    {
        return "nodes not registered";
    }
    auto stake=n->get_user_stake(t.senderAddress);
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

    auto u = t.getAddressState(t.senderAddress);
    t.value+=amount;

    n->sub_stake(t.senderAddress, amount);

    v->setDirty(t.roll);
    n->setDirty(t.roll);
    u->setDirty(t.roll);

    t.gasUsed+=v->getGas("node_unstake");

    b.emit_command(t.tx_id, seqId, "node_unstake",R"({"node":"%s","from":"%s","amount":"%s"})",
        node.container.c_str(),
        base16::encode(t.senderAddress.addr).c_str(),
        std::to_string(amount).c_str()
    );

    return std::nullopt;
}

std::optional<std::string> TR::execute_node_enable(yyjson_val *params, b_params & b,t_params &t,
     int seqId)
{
    MUTEX_INSPECTOR;
    auto v = b.db->getValuesNoCreate();
    NODE_id node;
    auto err=yy_get_string(params,"node",node.container);
    if(err)return err;

    auto n = t.getNode(node);
    if (!n.valid())
    {
        return "node not found";
    }
    if (n->get_owner() != t.senderAddress)
    {
        return "only node owner can enable node owner "+base16::encode(n->get_owner().addr)+" " + base16::encode(t.senderAddress.addr);
    }
    n->setDirty(t.roll);

    t.gasUsed+=v->getGas("node_enable");

    // t.logMsg(txid, seqId, "node %s enabled", node.container.c_str());
    b.emit_command(t.tx_id, seqId, "node_enable",R"({"node":"%s"})", node.container.c_str());

    return std::nullopt;
}




std::optional<std::string> TR::execute_contract_deploy(yyjson_val *params, b_params &b, t_params &t,
    int seqId)
{
    MUTEX_INSPECTOR;
    auto v = b.db->getValuesNoCreate();
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
    auto nn = b.db->getContract(cn);
    if (nn.valid())
        return "Contract already registered with name";

    auto us = t.getAddressState(t.senderAddress);
    if (!us.valid())
        return "if(!us.valid())";


    auto n = b.db->addContract(cn, t.roll);

    std::string src;
     err=yy_get_string(params,"src",src);
    if(err)return err;

    {
        M_LOCK(n->parent->mx);
        n->src=src;
        n->owner=t.senderAddress;
    }

    n->setDirty(t.roll);
    us->setDirty(t.roll);

    t.gasUsed+=v->getGas("contract_deploy");

    b.emit_command(t.tx_id, seqId, "contract_deploy",R"({"name":"%s","owner":"%s"})",name.c_str(),base16::encode(t.senderAddress.addr).c_str());

    return std::nullopt;
}
std::optional<std::string> TR::execute_contract_update(yyjson_val *params, b_params &b, t_params &t,
     int seqId)
{
    MUTEX_INSPECTOR;
    auto v = b.db->getValuesNoCreate();
    CONTRACT_id cn;
    auto err=yy_get_string(params,"name",cn.container);
    if(err)
        return err;

    auto n = b.db->getContract(cn);
    if (!n.valid())
        return "contract not exists";

    if(t.senderAddress!=n->owner)
    {
        return "sender is not contract owner";
    }
    auto us = t.getAddressState(t.senderAddress);
    if (!us.valid())
        return "if(!us.valid())";
    


    // auto n = t.root->addContract(name, by);
    std::string src;
    err=yy_get_string(params,"src",src);
    if(err) return err;

    {
        M_LOCK(n->parent->mx);
        n->src=src;
    }

    n->setDirty(t.roll);
    us->setDirty(t.roll);

    t.gasUsed+=v->getGas("contract_update");

    b.emit_command(t.tx_id, seqId, "contract_update",R"({"name":"%s","owner":"%s"})",cn.container.c_str(),base16::encode(t.senderAddress.addr).c_str());

    return std::nullopt;
}
