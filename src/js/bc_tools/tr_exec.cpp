#include <fcntl.h>

std::optional<std::string> TR::execute_mint(const yyjson::Value &params, t_params &t, const std::string &senderAddress, const REF_getter<fee_calcer> &by, const THASH_id &txid, int seqId)
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

    auto u = t.root->getUserState(senderAddress);
    if (!u.valid())
    {
        return "mint: sender state not found";
    }
    u->balance += amount;
    u->setDirty();
    t.addCalcer(u.get(),by);

    t.fee[senderAddress] += v->getFee("mint");

    t.logMsg(txid, seqId, "amount %s sucessfully minted on your balance", amount.toString().c_str());

    return std::nullopt;
}
std::optional<std::string> TR::execute_transfer(const yyjson::Value &params, t_params &t, const std::string &senderAddress, const REF_getter<fee_calcer> &by, const THASH_id &txid, int seqId)
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
    auto to_addr=base16::decode(_to.toString());
    if(to_addr.size()!=senderAddress.size())
        return "param to has invalid size";

    auto u = t.root->getUserState(senderAddress);
    if (!u.valid())
    {
        return "sender userstate invalid";
    }
    // auto to_addr = params["to"].get<std::string>();
    if (to_addr == senderAddress)
    {
        return "cannot transfer to self";
    }
    if (to_addr.size() != senderAddress.size())
    {
        return "invalid destination address";
    }
    auto to = t.root->getUserState(to_addr);
    if (!to.valid())
    {
        return "destination user not found";
    }
    auto fee=v->getFee("transfer");
    if (u->balance < fee + amount)
    {
        return "Not enough funds";
    }
    u->balance -= amount;
    to->balance += amount;
    u->setDirty();
    to->setDirty();
    t.addCalcer(u.get(),by);
    t.addCalcer(to.get(),by);


    t.fee[senderAddress] += fee;

    t.logMsg(txid, seqId, "amount %s sucessfully transferred to %s", amount.toString().c_str(), base16::encode(to_addr).c_str());

    return std::nullopt;
}
std::optional<std::string> TR::execute_node_update(const yyjson::Value &params, t_params &t, const std::string &senderAddress, const REF_getter<fee_calcer> &by, const THASH_id &txid, int seqId)
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
    if(nn->owner_ed_pk!=senderAddress)
    {
        return "only node owner can update node info";
    }
    auto us = t.root->getUserState(senderAddress);
    if (!us.valid())
        return "if(!us.valid())";
    auto fee=v->getFee("node_update");
    if (us->balance < fee)
        return "Not enough funds";



    auto ip=params / "ip";
    if (ip.isString())
    {
        nn->ip = ip.toString();
        t.logMsg(txid, seqId, "ip changed");
    }
    auto pk_ed=params / "pk_ed";
    if(pk_ed.isString())
    {
        nn->ed_pk = base16::decode(pk_ed.toString());
        t.logMsg(txid, seqId, "pk ed changed");
    }
    auto pk_bls=params / "pk_bls";
    if(pk_bls.isString())
    {
        nn->bls_pk.deserializeHexStr(pk_bls.toString());
        t.logMsg(txid, seqId, "pk bls changed");
    }


    nn->setDirty();
    us->setDirty();
    t.addCalcer(nn.get(),by);
    t.addCalcer(us.get(),by);


    t.fee[senderAddress] += fee;

    t.logMsg(txid, seqId, "node %s updated", name.container.c_str());

    return std::nullopt;
}

std::optional<std::string> TR::execute_node_create(const yyjson::Value &params, t_params &t, const std::string &senderAddress, const REF_getter<fee_calcer> &by, const THASH_id &txid, int seqId)
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

    auto us = t.root->getUserState(senderAddress);
    if (!us.valid())
        return "if(!us.valid())";
    auto fee=v->getFee("node_create");
    if (us->balance < fee)
        return "Not enough funds";


    auto n = t.root->addNode(name, by);

    auto ip=params / "ip";
    auto pk_ed=params / "pk_ed";
    auto pk_bls=params / "pk_bls";
    if(!ip.isString())
        return "string param ip required";
    if(!pk_ed.isString())
        return "string param pk_ed required";
    if(!pk_bls.isString())
        return "string param pk_bls required";

    n->name_ = name;
    n->ip = ip.toString();
    n->ed_pk = base16::decode(pk_ed.toString());
    n->bls_pk.deserialize(base16::decode(pk_bls.toString()));
    n->owner_ed_pk = senderAddress;
    n->setDirty();
    // u->setDirty();
    us->setDirty();
    t.addCalcer(n.get(),by);
    t.addCalcer(us.get(),by);



    t.fee[senderAddress] += fee;

    t.logMsg(txid, seqId, "node %s registered", name.container.c_str());

    return std::nullopt;
}
std::optional<std::string> TR::execute_node_stake(const yyjson::Value &params, t_params & t,const std::string& senderAddress, const REF_getter<fee_calcer>& by, const THASH_id& txid, int seqId)
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

    auto us = t.root->getUserState(senderAddress);
    if (!us.valid())
        return "if(!us.valid())";
    auto fee=v->getFee("node_stake");
    if (us->balance < amount + fee)
    {
        return "Insufficient funds";
    }

    auto n=t.root->getNode(node);
    if(!n.valid())
    {
        return "node not found";
    }
    BigInt &nodeStake = n->stakes[senderAddress];

    us->balance -= amount;
    nodeStake += amount;
    n->total_stake += amount;
    v->total_staked += amount;

    t.fee[senderAddress] += fee;

    t.logMsg(txid, seqId, "node %s staked on amount %s", node.container.c_str(), amount.toString().c_str());
    n->setDirty();
    us->setDirty();
    t.addCalcer(n.get(),by);
    t.addCalcer(us.get(),by);

    return std::nullopt;
}
std::optional<std::string> TR::execute_unstake_node(const yyjson::Value &params, t_params & t,const std::string& senderAddress, const REF_getter<fee_calcer>& by, const THASH_id& txid, int seqId)
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
    auto nodeStakeIt = n->stakes.find(senderAddress);
    if (nodeStakeIt == n->stakes.end())
    {
        return "no_stake_found";
    }
    auto &nodeStake = nodeStakeIt->second;
    if (nodeStake < amount)
    {
        return "insufficient stake in node";
    }

    auto u = t.root->getUserState(senderAddress);

    if (!u.valid())
        return "FATAL:  dst addr not found";
    auto fee=v->getFee("node_unstake");
    if(u->balance < fee)
    {
        return "Insufficient funds to unstake";
    }
    u->balance += amount;

    nodeStake -= amount;

    n->total_stake -= amount;

    v->total_staked -= amount;

    v->setDirty();
    n->setDirty();
    u->setDirty();
    t.addCalcer(v.get(),by);
    t.addCalcer(n.get(),by);
    t.addCalcer(u.get(),by);

    t.fee[senderAddress] += fee;

    t.logMsg(txid, seqId, "node %s unstaked on amount %s", node.container.c_str(), amount.toString().c_str());

    return std::nullopt;
}

std::optional<std::string> TR::execute_node_enable(const yyjson::Value &params, t_params & t,const std::string& senderAddress, const REF_getter<fee_calcer>& by, const THASH_id& txid, int seqId)
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
    if (n->owner_ed_pk != senderAddress)
    {
        return "only node owner can enable node";
    }
    auto fee=v->getFee("node_enable");
    auto us = t.root->getUserState(senderAddress);
    if (!us.valid())
        return "if(!us.valid())";
    if (us->balance < fee)
    {
        return "Insufficient funds";
    }
    n->missed_rounds = 0;
    n->setDirty();

    t.fee[senderAddress] += fee;

    t.logMsg(txid, seqId, "node %s enabled", node.container.c_str());

    return std::nullopt;
}

// std::optional<std::string> TR::execute(const tx::createContract &c, t_params &t, const std::string &senderAddress, const REF_getter<fee_calcer> &by, const THASH_id &txid, int seqId)
// {
//     auto v = t.root->getValues();
//     auto u = t.root->getUser(senderAddress);
//     if (!u.valid())
//         return "sender not found";

//     auto cc = t.root->getContract(c.name);
//     if (cc.valid())
//         return "contract name already exists";

//     cc = t.root->addContract(c.name, by);

//     cc->owner = senderAddress;
//     cc->name_ = c.name;
//     cc->src = c.src;

//     u->contracts.insert(c.name);
//     u->setDirty();
//     cc->setDirty();

//     t.fee[senderAddress] += v->fees[bc_values::contract_deploy];

//     t.logMsg(txid, seqId, "contract %s deployed successfully", c.name.c_str());

//     return std::nullopt;
//     return std::nullopt;

// }

// std::optional<std::string> TR::execute(const tx::createContract &c, t_params &t, const std::string &senderAddress, const REF_getter<fee_calcer> &by, const THASH_id &txid, int seqId)
// {
//     auto v = t.root->getValues();
//     auto u = t.root->getUser(senderAddress);
//     if (!u.valid())
//         return "sender not found";

//     auto cc = t.root->getContract(c.name);
//     if (cc.valid())
//         return "contract name already exists";

//     cc = t.root->addContract(c.name, by);

//     cc->owner = senderAddress;
//     cc->name_ = c.name;
//     cc->src = c.src;

//     u->contracts.insert(c.name);
//     u->setDirty();
//     cc->setDirty();

//     t.fee[senderAddress] += v->fees[bc_values::contract_deploy];

//     t.logMsg(txid, seqId, "contract %s deployed successfully", c.name.c_str());

//     return std::nullopt;
// }



