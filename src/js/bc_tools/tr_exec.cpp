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
    
        // return "'"+(std::string)key+"' must be string";
    if(yyjson_is_str(_name))
        out.from_string(yyjson_get_str(_name));
    else if(yyjson_is_num(_name))
        out=yyjson_get_uint(_name);
    else return "'"+(std::string)key+"' must be string or num";

    return std::nullopt;

}

std::optional<std::string> TR::execute_mint(yyjson_val *params, t_params &t, 
        const ADDRESS_id &senderAddress,  const THASH_id &txid, int seqId, const EPOCH_id& epoch)
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

    

    auto u = t.root->getAddressState(senderAddress);
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
    // t.addCalcer(u.get(),by);
    auto gas=v->getGas("mint");
    if(t.gas_remains<gas)
        return "gas exceeds limit";
    t.gas_remains-=gas;

    t.emit_command(txid, seqId,"mint",R"({"to":"%s","amount":"%s"})",
        base16::encode(senderAddress.addr).c_str(),
        amount.toString().c_str());

    return std::nullopt;
}
std::optional<std::string> TR::execute_transfer(yyjson_val *params, t_params &t, 
    const ADDRESS_id &senderAddress,  const THASH_id &txid, int seqId, const EPOCH_id& epoch)
{
    auto v = t.root->getValues();


    BigInt amount=0;
    auto err=yy_get_bn(params,"amount",amount);
    if(err) return err;

    std::string to_s;
    err=yy_get_string(params,"to",to_s);
    if(err) return err;

    ADDRESS_id to_addr;

    to_addr.addr=base16::decode(to_s);
    if(to_addr.addr.size()!=senderAddress.addr.size())
        return "param to has invalid size";

    auto u = t.root->getAddressState(senderAddress);
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

    auto to = t.root->getAddressState(to_addr);
    if (!to.valid())
    {
        return "destination user not found";
    }
    auto gas=v->getGas("transfer");
    if(t.gas_remains<gas)
        return "gas exceeds limit";
    if(amount>t.value)
        return "transfer amount exeeds value";
    t.gas_remains-=gas;
    t.value-=amount;
    {
        M_LOCK(to->parent->mx);
        to->balance+=amount;
    }
    u->setDirty(epoch);
    to->setDirty(epoch);

    t.emit_command(txid, seqId, "transfer", R"({"from":"%s","to":"%s","amount":"%s"})", 
        base16::encode(senderAddress.addr).c_str(), 
        base16::encode(to_addr.addr).c_str(), 
        amount.toString().c_str()
        );

    return std::nullopt;
}
std::optional<std::string> TR::execute_node_update(yyjson_val *params, t_params &t, 
    const ADDRESS_id &senderAddress,  const THASH_id &txid, int seqId, const EPOCH_id& epoch)
{
    // if(senderAddress!=)
    auto v = t.root->getValues();
    NODE_id name;
    auto err=yy_get_string(params,"name",name.container);
    if(err) return err;
    std::string ip;
    err=yy_get_string(params,"ip",ip);
    if(err) return err;
    


    auto nn = t.root->getNode(name);
    if (!nn.valid())
        return "Node not found";
    if(nn->get_owner()!=senderAddress)
    {
        return "only node owner can update node info";
    }
    auto us = t.root->getAddressState(senderAddress);
    if (!us.valid())
        return "if(!us.valid())";
    auto gas=v->getGas("node_update");
    if(t.gas_remains<gas)
        return "gas exeeds limit";
    t.gas_remains-=gas;



    nn->set_ip(ip);
    t.emit_command(txid, seqId, "node_change_ip",R"({"node":"%s","ip":"%s"})",name.container.c_str(),ip.c_str());


    nn->setDirty(epoch);
    us->setDirty(epoch);

    return std::nullopt;
}
std::optional<std::string> TR::execute_node_create(yyjson_val *params, t_params &t, 
    const ADDRESS_id &senderAddress,  const THASH_id &txid, int seqId, const EPOCH_id& epoch)
{
    auto v = t.root->getValues();
    NODE_id name;
    auto err=yy_get_string(params,"name",name.container);
    if(err)
        return err;
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

    auto us = t.root->getAddressState(senderAddress);
    if (!us.valid())
        return "if(!us.valid())";
    auto gas=v->getGas("node_create");
    if(t.gas_remains<gas)
        return "gas exeeds limit";
    t.gas_remains-=gas;
    auto n = t.root->addNode(name, epoch);

    std::string ip,pk_ed,pk_bls;
    err=yy_get_string(params,"ip",ip);
    if(err)return err;
    err=yy_get_string(params,"pk_ed",pk_ed);
    if(err)return err;
    err=yy_get_string(params,"pk_bls",pk_bls);
    if(err)return err;

        blst_cpp::PublicKey bls;
        bls.deserializeHexStr(pk_bls);
    n->init(name, 
        senderAddress, 
        bls, 
        base16::decode(pk_ed), ip);
    n->setDirty(epoch);
    us->setDirty(epoch);
    t.emit_command(txid, seqId, "node_create",R"({"node":"%s","ip":"%s"})",name.container.c_str(),ip.c_str());

    return std::nullopt;
}
std::optional<std::string> TR::execute_node_stake(yyjson_val *params, t_params & t,
    const ADDRESS_id& senderAddress,  const THASH_id& txid, int seqId, const EPOCH_id& epoch)
{
    auto v = t.root->getValues();
    // BigInt amount;
    NODE_id node;
    auto err=yy_get_string(params,"node",node.container);
    if(err)
        return err;
    BigInt amount;
    err=yy_get_bn(params,"amount",amount);
    if(err)
        return err;

    if(t.value<amount)
        return "value<amount";

    auto us = t.root->getAddressState(senderAddress);
    if (!us.valid())
        return "if(!us.valid())";
    auto gas=v->getGas("node_stake");
    if(t.gas_remains<gas)
        return "gas exceeds limit";
    t.gas_remains-=gas;

    auto n=t.root->getNode(node);
    if(!n.valid())
    {
        return "node not found";
    }
    logErr2("n owner %s sender %s", base16::encode(n->get_owner().addr).c_str(), base16::encode(senderAddress.addr).c_str());

    {
    }
    // auto stake=t.value;
    n->add_stake(senderAddress, amount);
    t.value-=amount;
    t.emit_command(txid, seqId, "node_stake",R"({"node":"%s","stake":"%s","from":"%s"})",
        node.container.c_str(),
        amount.toString().c_str(),
        base16::encode(senderAddress.addr).c_str()
    );
    n->setDirty(epoch);
    us->setDirty(epoch);

    return std::nullopt;
}
std::optional<std::string> TR::execute_unstake_node(yyjson_val *params, t_params & t,
    const ADDRESS_id& senderAddress,  const THASH_id& txid, int seqId, const EPOCH_id& epoch)
{
    auto v = t.root->getValues();

    NODE_id node;
    auto err=yy_get_string(params,"node",node.container);
    if(err)
        return err;

    BigInt amount;
    err=yy_get_bn(params,"amount",amount);
    if(err)
        return err;

    auto n = t.root->getNode(node);
    if (!n.valid())
    {
        return "nodes not registered";
    }
    auto stake=n->get_user_stake(senderAddress);
    // auto nodeStakeIt = n->stakes.find(senderAddress);
    if (stake < amount)
    {
        return "not enough node stake (stake < amount)";
    }

    auto u = t.root->getAddressState(senderAddress);

    if (!u.valid())
        return "FATAL:  dst addr not found";
    auto gas=v->getGas("node_unstake");
    if(t.gas_remains<gas)
        return "gas exceeds limit";
    t.gas_remains-=gas;
    
    
    {
        M_LOCK(u->parent->mx);
        u->balance+=amount;

    }

    n->sub_stake(senderAddress, amount);
    v->setDirty(epoch);
    n->setDirty(epoch);
    u->setDirty(epoch);

    t.emit_command(txid, seqId, "node_unstake",R"({"node":"%s","from":"%s","amount":"%s"})",
        node.container.c_str(),
        base16::encode(senderAddress.addr).c_str(),
        amount.toString().c_str()
    );

    return std::nullopt;
}

std::optional<std::string> TR::execute_node_enable(yyjson_val *params, t_params & t,
    const ADDRESS_id& senderAddress,  const THASH_id& txid, int seqId, const EPOCH_id& epoch)
{
    auto v = t.root->getValues();

        NODE_id node;
    auto err=yy_get_string(params,"node",node.container);
    if(err)
        return err;

    // auto _node = params / "node";
    // if (!_node.isString())
    // {
    //     return "param string node required";
    // }
    // NODE_id node;
    // node.container = _node.toString();

    auto n = t.root->getNode(node);
    if (!n.valid())
    {
        return "node not found";
    }
    if (n->get_owner() != senderAddress)
    {
        return "only node owner can enable node owner "+base16::encode(n->get_owner().addr)+" " + base16::encode(senderAddress.addr);
    }
    auto gas=v->getGas("node_enable");
    if(t.gas_remains<gas)
        return "gas exceeds limit";
    t.gas_remains-=gas;

    auto us = t.root->getAddressState(senderAddress);
    if (!us.valid())
        return "if(!us.valid())";
    // {
        // M_LOCK(us->parent->mx);     
        // if (us->balance < fee)
        // {
        //     return "Insufficient funds";
        // }
    // }
    n->reset_missed_rounds();
    n->setDirty(epoch);

    // t.fee[senderAddress] += fee;

    // t.logMsg(txid, seqId, "node %s enabled", node.container.c_str());
    t.emit_command(txid, seqId, "node_enable",R"({"node":"%s"})", node.container.c_str());

    return std::nullopt;
}




std::optional<std::string> TR::execute_contract_deploy(yyjson_val *params, t_params &t, 
    const ADDRESS_id &senderAddress,  const THASH_id &txid, int seqId, const EPOCH_id& epoch)
{
    auto v = t.root->getValues();

    CONTRACT_id name;
    auto err=yy_get_string(params,"name",name.container);
    if(err)
        return err;

    // std::string name;
    // auto _name=params / "name";
    // if(!_name.isString())
    // {
    //     return "no string param name";
    // }
    // name = _name.toString();
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
    // CONTRACT_id cn;
    // cn.container=name;
    auto nn = t.root->getContract(name);
    if (nn.valid())
        return "Contract already registered with name";

    auto us = t.root->getAddressState(senderAddress);
    if (!us.valid())
        return "if(!us.valid())";
    auto gas=v->getGas("contract_deploy");
    if(t.gas_remains<gas)
        return "gas limit exceeded";
    t.gas_remains-=gas;
    // {
    //     M_LOCK(us->parent->mx);
    //     if (us->balance < fee)
    //         return "Not enough funds";
    // }


    auto n = t.root->addContract(name, epoch);

    std::string src;
    err=yy_get_string(params,"src",src);
    if(err) return err;
    // auto src=params / "src";
    // if(!src.isString())
    //     return "string param src required";

    {
        M_LOCK(n->parent->mx);
        n->src=src;
        n->owner=senderAddress;
    }

    n->setDirty(epoch);
    us->setDirty(epoch);
    // t.addCalcer(n.get(),by);
    // t.addCalcer(us.get(),by);

    // t.fee[senderAddress] += fee;

    t.emit_command(txid, seqId, "contract_deploy",R"({"name":"%s","owner":"%s"})",name.container.c_str(),base16::encode(senderAddress.addr).c_str());

    return std::nullopt;
}
std::optional<std::string> TR::execute_contract_update(yyjson_val *params, t_params &t, 
    const ADDRESS_id &senderAddress,  const THASH_id &txid, int seqId, const EPOCH_id& epoch)
{
    auto v = t.root->getValues();

    CONTRACT_id cn;
    auto err=yy_get_string(params,"name",cn.container);
    if(err) return err;
    // std::string name;
    // auto _name=params / "name";
    // if(!_name.isString())
    // {
    //     return "no string param name";
    // }
    // name = _name.toString();
    // CONTRACT_id cn;
    // cn.container=name;
    auto n = t.root->getContract(cn);
    if (!n.valid())
        return "contract not exists";

    if(senderAddress!=n->owner)
    {
        return "sender is not contract owner";
    }
    auto us = t.root->getAddressState(senderAddress);
    if (!us.valid())
        return "if(!us.valid())";
    auto gas=v->getGas("contract_update");
    if(t.gas_remains<gas)
        return "gas exceeds limit";
    t.gas_remains-=gas;
    // {
    //     M_LOCK(us->parent->mx);
    //     if (us->balance < fee)
    //         return "Not enough funds";
    // }
    


    // auto n = t.root->addContract(name, by);
    std::string src;
    err=yy_get_string(params,"src",src);
    if(err) return err;

    // auto src=params / "src";
    // if(!src.isString())
    //     return "string param src required";

    {
        M_LOCK(n->parent->mx);
        n->src=src;
    }

    n->setDirty(epoch);
    us->setDirty(epoch);
    // t.addCalcer(n.get(),by);
    // t.addCalcer(us.get(),by);

    // t.fee[senderAddress] += fee;

    t.emit_command(txid, seqId, "contract_update",R"({"name":"%s","owner":"%s"})",cn.container.c_str(),base16::encode(senderAddress.addr).c_str());

    return std::nullopt;
}
