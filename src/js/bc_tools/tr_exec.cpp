#include "NODE_id.h"
#include "base62.h"
#include "msg.h"
#include <fcntl.h>
#include "cellable.h"
#include "tr_exec.h"
#include "msg_tx.h"

std::optional<std::string> TR::execute_mint(const nlohmann::json &params, t_params &t, const std::string &senderAddress, const REF_getter<fee_calcer> &by, const THASH_id &txid, int seqId)
{
    auto v = t.root->getValues();
    auto it = v->emitters_hex.find(senderAddress);
    if (it == v->emitters_hex.end())
    {
        logErr2("insufficient_privileges");
        return "insufficient_privileges";
    }

    if (!params.contains("amount"))
    {
        logErr2("param amount required");
        return "param amount required";
    }
    BigInt amount;
    auto &a = params["amount"];
    if (a.is_number_integer())
    {
        amount = a.get<uint64_t>();
    }
    else if (a.is_string())
    {
        amount.from_string(a.get<std::string>());
    }
    else
        return "param amount must be number or string";

    auto u = t.root->getUserState(senderAddress);
    if (!u.valid())
    {
        throw CommonError("if(!u.valid())");
    }
    u->balance += amount;
    u->setDirty(by);

    t.fee[senderAddress] += v->fees[bc_values::mint];

    t.logMsg(txid, seqId, "amount %s sucessfully minted on your balance", amount.toString().c_str());

    return std::nullopt;
}
std::optional<std::string> TR::execute_transfer(const nlohmann::json &params, t_params &t, const std::string &senderAddress, const REF_getter<fee_calcer> &by, const THASH_id &txid, int seqId)
{
    auto v = t.root->getValues();

    if (!params.contains("amount"))
    {
        logErr2("param amount required");
        return "param amount required";
    }
    BigInt amount;
    auto &a = params["amount"];
    if (a.is_number_integer())
    {
        amount = a.get<uint64_t>();
    }
    else if (a.is_string())
    {
        amount.from_string(a.get<std::string>());
    }
    else
        return "param amount must be number or string";
    // if()
    auto u = t.root->getUserState(senderAddress);
    if (!u.valid())
    {
        throw CommonError("if(!u.valid())");
    }
    auto to_addr = params["to"].get<std::string>();
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
    if (u->balance < v->fees[bc_values::transfer] + amount)
    {
        return "Not enough money";
    }
    u->balance -= amount;
    to->balance += amount;
    u->setDirty(by);
    to->setDirty(by);

    t.fee[senderAddress] += v->fees[bc_values::transfer];

    t.logMsg(txid, seqId, "amount %s sucessfully transferred to %s", amount.toString().c_str(), to_addr.c_str());

    return std::nullopt;
}
std::optional<std::string> TR::execute_update_node(const nlohmann::json &params, t_params &t, const std::string &senderAddress, const REF_getter<fee_calcer> &by, const THASH_id &txid, int seqId)
{
    auto v = t.root->getValues();
    NODE_id name;
    if (!params.contains("name"))
        return "param name required";
    name.container = params["name"].get<std::string>();
    // for (auto &z : name.container)
    // {
    //     if (!isalnum(z))
    //     {
    //         return "allowed only isalnum symbols";
    //     }
    //     if (isalpha(z) && isupper(z))
    //     {
    //         return "allowed only lowercase symbols";
    //     }
    // }

    auto nn = t.root->getNode(name);
    if (!nn.valid())
        return "Node not found";

    auto us = t.root->getUserState(senderAddress);
    if (!us.valid())
        return "if(!us.valid())";
    // auto u = t.root->getUser(senderAddress);
    // if (!u.valid())
    //     return "if(!u.valid())";

    if (us->balance < v->fees[bc_values::node_update])
        return "Not enough funds";

    // u->nodes.insert(name);

    if (senderAddress != nn->owner_ed_pkhex)
    {
        return "only node owner can update node info";
    }

    // auto n = t.root->addNode(name, by);
    if (params.contains("ip") && params["ip"].is_string())
    {
        nn->ip = params["ip"].get<std::string>();
        t.logMsg(txid, seqId, "ip changed");
    }
    if (params.contains("pk_ed") && params["pk_ed"].is_string())
    {
        auto ed_pk = base62::decode(params["pk_ed"].get<std::string>());

        if (ed_pk.size() != senderAddress.size() / 2)
        {
            return "invalid ed pk";
        }
        nn->ed_pk = ed_pk;
        t.logMsg(txid, seqId, "pk ed changed");
    }
    if (params.contains("pk_bls") && params["pk_bls"].is_string())
    {
        nn->bls_pk.deserialize(base62::decode(params["pk_bls"].get<std::string>()));
        t.logMsg(txid, seqId, "pk bls changed");
    }

    // if (!params.contains("ip") || !params["ip"].is_string())
    //     return "param ip required";
    // if (!params.contains("pk_ed") || !params["pk_ed"].is_string())
    //     return "param pk_ed required";
    // if (!params.contains("pk_bls") || !params["pk_bls"].is_string())
    //     return "param pk_bls required";

    // n->name_ = name;
    // n->ip = params["ip"].get<std::string>();
    // n->ed_pk = base62::decode(params["pk_ed"].get<std::string>());
    // if (n->ed_pk.size() != senderAddress.size() / 2)
    // {
    //     return "invalid ed pk";
    // }
    // n->bls_pk.deserialize(base62::decode(params["pk_bls"].get<std::string>()));
    // n->owner_ed_pkhex = senderAddress;
    nn->setDirty(by);
    // u->setDirty(by);
    us->setDirty(by);

    t.fee[senderAddress] += v->fees[bc_values::node_update];

    t.logMsg(txid, seqId, "node %s updated", name.container.c_str());

    return std::nullopt;
}

std::optional<std::string> TR::execute_create_node(const nlohmann::json &params, t_params &t, const std::string &senderAddress, const REF_getter<fee_calcer> &by, const THASH_id &txid, int seqId)
{
    auto v = t.root->getValues();
    NODE_id name;
    if (!params.contains("name"))
        return "param name required";
    name.container = params["name"].get<std::string>();
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
    auto u = t.root->getUser(senderAddress);
    if (!u.valid())
        return "if(!u.valid())";

    if (us->balance < v->fees[bc_values::node_create])
        return "Not enough funds";

    u->nodes.insert(name);

    auto n = t.root->addNode(name, by);

    if (!params.contains("ip") || !params["ip"].is_string())
        return "param ip required";
    if (!params.contains("pk_ed") || !params["pk_ed"].is_string())
        return "param pk_ed required";
    if (!params.contains("pk_bls") || !params["pk_bls"].is_string())
        return "param pk_bls required";

    n->name_ = name;
    n->ip = params["ip"].get<std::string>();
    n->ed_pk = base62::decode(params["pk_ed"].get<std::string>());
    if (n->ed_pk.size() != senderAddress.size() / 2)
    {
        return "invalid ed pk";
    }
    n->bls_pk.deserialize(base62::decode(params["pk_bls"].get<std::string>()));
    n->owner_ed_pkhex = senderAddress;
    n->setDirty(by);
    u->setDirty(by);
    us->setDirty(by);

    t.fee[senderAddress] += v->fees[bc_values::node_create];

    t.logMsg(txid, seqId, "node %s registered", name.container.c_str());

    return std::nullopt;
}

std::optional<std::string> TR::execute(const tx::unstake &c, t_params &t, const std::string &senderAddress, const REF_getter<fee_calcer> &by, const THASH_id &txid, int seqId)
{
    auto v = t.root->getValues();

    auto n = t.root->getNode(c.node);
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
    if (nodeStake < c.amount)
    {
        return "insufficient stake";
    }

    auto u = t.root->getUser(senderAddress);

    if (!u.valid())
        return "FATAL:  dst addr not found";

    nodeStake -= c.amount;
    v->total_staked -= c.amount;
    v->setDirty(by);
    n->setDirty(by);
    u->setDirty(by);

    t.fee[senderAddress] += v->fees[bc_values::unstake];

    t.logMsg(txid, seqId, "node %s unstaked on amount %s", c.node.container.c_str(), c.amount.toString().c_str());

    return std::nullopt;
}
std::optional<std::string> TR::execute(const tx::createContract &c, t_params &t, const std::string &senderAddress, const REF_getter<fee_calcer> &by, const THASH_id &txid, int seqId)
{
    auto v = t.root->getValues();
    auto u = t.root->getUser(senderAddress);
    if (!u.valid())
        return "sender not found";

    auto cc = t.root->getContract(c.name);
    if (cc.valid())
        return "contract name already exists";

    cc = t.root->addContract(c.name, by);

    cc->owner = senderAddress;
    cc->name_ = c.name;
    cc->src = c.src;

    u->contracts.insert(c.name);
    u->setDirty(by);
    cc->setDirty(by);

    t.fee[senderAddress] += v->fees[bc_values::contract_deploy];

    t.logMsg(txid, seqId, "contract %s deployed successfully", c.name.c_str());

    return std::nullopt;
}

std::optional<std::string> TR::execute(const tx::transfer &c, t_params &t, const std::string &senderAddress, const REF_getter<fee_calcer> &by, const THASH_id &txid, int seqId)
{
    auto v = t.root->getValues();
    auto from = t.root->getUserState(senderAddress);
    if (!from.valid())
        return "cannot find src addr";

    auto to = t.root->getUserState(c.to_address);
    if (!to.valid())
    {
        return "destination user not found " + base62::encode(c.to_address);
    }
    if (from->balance < v->fees[bc_values::transfer] + c.amount)
    {
        return "Not enough money";
    }
    from->balance -= c.amount;
    to->balance += c.amount;
    from->setDirty(by);
    to->setDirty(by);
    t.fee[senderAddress] += v->fees[bc_values::transfer];
    t.logMsg(txid, seqId, "amount %s successfully transferred", c.amount.toString().c_str());
    return std::nullopt;
}

std::optional<std::string> TR::execute(const tx::stake &c, t_params &t, const std::string &senderAddress, const REF_getter<fee_calcer> &by, const THASH_id &txid, int seqId)
{
    auto v = t.root->getValues();
    if (!v.valid())
    {
        return "values not found";
    }
    auto n = t.root->getNode(c.node);
    if (!n.valid())
    {
        return "Node not registered";
    }
    auto sender = t.root->getUserState(senderAddress);
    if (!sender.valid())
    {
        return "user account not found";
    }

    // BigInt amt;
    // amt.from_string(c.amount);
    if (sender->balance + v->fees[bc_values::stake] < c.amount)
    {
        return "Insufficient funds";
    }

    BigInt &nodeStake = n->stakes[senderAddress];

    sender->balance -= c.amount;
    nodeStake += c.amount;
    t.fee[senderAddress] += v->fees[bc_values::stake];

    t.logMsg(txid, seqId, "node %s staked on amount %s", c.node.container.c_str(), c.amount.toString().c_str());
    n->setDirty(by);
    sender->setDirty(by);
    return std::nullopt;
}

std::optional<std::string> TR::execute(const tx::registerNode &c, t_params &t, const std::string &senderAddress, const REF_getter<fee_calcer> &by, const THASH_id &txid, int seqId)
{
    auto v = t.root->getValues();
    for (auto &z : c.name.container)
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

    auto nn = t.root->getNode(c.name);
    if (nn.valid())
        return "Node already registered with name";
    auto us = t.root->getUserState(senderAddress);
    if (!us.valid())
        return "if(!us.valid())";
    auto u = t.root->getUser(senderAddress);
    if (!u.valid())
        return "if(!u.valid())";
    if (us->balance < v->fees[bc_values::node_create])
        return "Not enough funds";

    u->nodes.insert(c.name);

    auto n = t.root->addNode(c.name, by);
    n->name_ = c.name;
    n->ip = c.ip;
    n->ed_pk = c.pk_ed;
    n->bls_pk = c.pk_bls;
    n->owner_ed_pkhex = senderAddress;
    n->setDirty(by);
    u->setDirty(by);
    us->setDirty(by);

    t.fee[senderAddress] += v->fees[bc_values::node_create];

    t.logMsg(txid, seqId, "node %s registered", c.name.container.c_str());

    return std::nullopt;
}
