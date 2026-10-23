#include <fcntl.h>
#include <optional>
#include <yyjson.h>
#include <string>
#include "commonError.h"
#include "tr_exec.h"
#include "ADDRESS_id.h"
#include "PK_id.h"
#include "yyjson_to_quickjs.h"
#include "QUORUM.h"

std::optional<std::string> TR::execute_mint(yyjson_val *params, b_params &b, t_params &t,
        int seqId)
{
    MUTEX_INSPECTOR;

    auto v = b.db->getValuesNoCreateConst();
    auto it = v->emitters_bin.find(t.senderAddress);
    if (it == v->emitters_bin.end())
    {
        logErr2("insufficient_privileges");
        return "insufficient_privileges";
    }
    uint64_t amount=0;
    auto err=yy_get_uint64_t(params,"amount",amount);
    if(err) return err;


    auto u = b.db->getAddressStateNoCreate(t.senderAddress);
    if (!u.valid())
    {
        return "mint: sender not found";
    }
    {
        M_LOCK(u->parent->mx);
        u->balance+=amount;
    }
    t.markDirty(u.get());

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
    auto v = b.db->getValuesNoCreateConst();

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

    auto u = b.db->getAddressStateNoCreateConst(t.senderAddress);
    if (!u.valid())
    {
        return "sender userstate invalid";
    }
    if (to_addr == t.senderAddress)
    {
        return "cannot transfer to self";
    }
    if (to_addr.addr.size() != t.senderAddress.addr.size())
    {
        return "invalid destination address";
    }
    auto to = b.db->getAddressStateOrCreate(to_addr,t.roll);
    if (!to.valid())
    {
        return "destination user not found";
    }
    if(amount>t.value)
        return "amount>t.value";

    t.value-=amount;
    {
        M_LOCK(to->parent->mx);
        to->balance+=amount;
    }
    t.markDirty(to.get());

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
    auto v = b.db->getValuesNoCreateConst();
    NODE_id name;
    auto err=yy_get_string(params,"name",name.container);
    if(err) return err;



    auto nn = b.db->getNodeNoCreate(name);
    if (!nn.valid())
        return "Node not found";
    if(nn->get_owner()!=t.senderAddress)
    {
        return "only node owner can update node info";
    }
    auto us = b.db->getAddressStateNoCreateConst(t.senderAddress);
    if (!us.valid())
        return "if(!us.valid())";

    std::string ip;
    err=yy_get_string(params,"ip",ip);
    if(err) return err;

    {
        nn->set_ip(ip);
        b.emit_command(t.tx_id, seqId, "node_change_ip",R"({"node":"%s","ip":"%s"})",name.container.c_str(),ip.c_str());

    }

    t.markDirty(nn.get());


    t.gasUsed+=v->getGas("node_update");


    return std::nullopt;
}

std::optional<std::string> TR::execute_node_create(yyjson_val *params, b_params &b, t_params &t,
        int seqId)
{
    MUTEX_INSPECTOR;
    auto v = b.db->getValuesNoCreateConst();
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

    auto nn = b.db->getNodeNoCreateConst(name);
    if (nn.valid())
        return "Node already registered with name";

    auto us = b.db->getAddressStateNoCreateConst(t.senderAddress);
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

    t.markDirty(n.get());



    t.gasUsed+=v->getGas("node_create");

    // t.logMsg(txid, seqId, "node %s registered", name.container.c_str());
    b.emit_command(t.tx_id, seqId, "node_create",R"({"node":"%s","ip":"%s"})",name.container.c_str(),ip.c_str());

    return std::nullopt;
}
std::optional<std::string> TR::execute_node_stake(yyjson_val *params, b_params & b,t_params &t,
        int seqId)
{
    MUTEX_INSPECTOR;
    auto v = b.db->getValuesNoCreateConst();

    uint64_t amount=0;
    auto err=yy_get_uint64_t(params,"amount",amount);
    if(err) return err;

    NODE_id node;
    err=yy_get_string(params,"node",node.container);
    if(err) return err;


    auto us = b.db->getAddressStateNoCreateConst(t.senderAddress);
    if (!us.valid())
        return "if(!us.valid())";

    auto n=b.db->getNodeNoCreate(node);
    if(!n.valid())
    {
        return "node not found";
    }
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
    t.markDirty(n.get());


    return std::nullopt;
}
std::optional<std::string> TR::execute_unstake_node(yyjson_val *params, b_params & b,t_params &t,
        int seqId)
{
    MUTEX_INSPECTOR;
    auto v = b.db->getValuesNoCreateConst();
    uint64_t amount=0;
    auto err=yy_get_uint64_t(params,"amount",amount);
    if(err) return err;
    NODE_id node;
    err=yy_get_string(params,"node",node.container);
    if(err) return err;


    auto n = b.db->getNodeNoCreate(node);
    if (!n.valid())
    {
        return "nodes not registered";
    }
    auto stake=n->get_user_stake(t.senderAddress);
    if (stake == 0)
    {
        return "no_stake_found";
    }
    if (stake < amount)
    {
        return "insufficient stake in node";
    }

    auto u = b.db->getAddressStateNoCreateConst(t.senderAddress);
    t.value+=amount;

    n->sub_stake(t.senderAddress, amount);

    t.markDirty(n.get());
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
    auto v = b.db->getValuesNoCreateConst();
    NODE_id node;
    auto err=yy_get_string(params,"node",node.container);
    if(err)return err;

    auto n = b.db->getNodeNoCreate(node);
    if (!n.valid())
    {
        return "node not found";
    }
    if (n->get_owner() != t.senderAddress)
    {
        return "only node owner can enable node owner "+base16::encode(n->get_owner().addr)+" " + base16::encode(t.senderAddress.addr);
    }
    if(n->isEnabled())
        return "node already enabled";

    n->setEnabled(1);
    t.markDirty(n.get());

    t.gasUsed+=v->getGas("node_enable");

    b.emit_command(t.tx_id, seqId, "node_enable",R"({"node":"%s"})", node.container.c_str());


    return std::nullopt;
}

std::optional<std::string> TR::execute_node_disable(yyjson_val *params, b_params & b,t_params &t,
        int seqId)
{
    MUTEX_INSPECTOR;
    auto v = b.db->getValuesNoCreateConst();
    NODE_id node;
    auto err=yy_get_string(params,"node",node.container);
    if(err)return err;

    auto n = b.db->getNodeNoCreate(node);
    if (!n.valid())
    {
        return "node not found";
    }
    if (n->get_owner() != t.senderAddress)
    {
        return "only node owner can enable node owner "+base16::encode(n->get_owner().addr)+" " + base16::encode(t.senderAddress.addr);
    }
    if(!n->isEnabled())
        return "node already disabled";

    n->setEnabled(0);
    t.markDirty(n.get());

    t.gasUsed+=v->getGas("node_enable");

    b.emit_command(t.tx_id, seqId, "node_disable",R"({"node":"%s"})", node.container.c_str());



    return std::nullopt;
}



std::optional<std::string> TR::execute_contract_deploy(yyjson_val *params, b_params &b, t_params &t,
        int seqId)
{
    MUTEX_INSPECTOR;
    auto v = b.db->getValuesNoCreateConst();
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
    auto nn = b.db->getContractNoCreateConst(cn);
    if (nn.valid())
        return "Contract already registered with name";

    auto us = b.db->getAddressStateNoCreateConst(t.senderAddress);
    if (!us.valid())
        return "if(!us.valid())";


    auto n = b.db->getContractOrCreate(cn, t.roll);

    std::string src;
    err=yy_get_string(params,"src",src);
    if(err)return err;

    {
        M_LOCK(n->parent->mx);
        n->src=src;
        n->owner=t.senderAddress;
    }

    t.markDirty(n.get());


    t.gasUsed+=v->getGas("contract_deploy");

    b.emit_command(t.tx_id, seqId, "contract_deploy",R"({"name":"%s","owner":"%s"})",name.c_str(),base16::encode(t.senderAddress.addr).c_str());

    return std::nullopt;
}
std::optional<std::string> TR::execute_contract_update(yyjson_val *params, b_params &b, t_params &t,
        int seqId)
{
    MUTEX_INSPECTOR;
    auto v = b.db->getValuesNoCreateConst();
    CONTRACT_id cn;
    auto err=yy_get_string(params,"name",cn.container);
    if(err)
        return err;

    auto n = b.db->getContractNoCreate(cn);
    if (!n.valid())
        return "contract not exists";

    if(t.senderAddress!=n->owner)
    {
        return "sender is not contract owner";
    }
    auto us = b.db->getAddressStateNoCreateConst(t.senderAddress);
    if (!us.valid())
        return "if(!us.valid())";



    std::string src;
    err=yy_get_string(params,"src",src);
    if(err) return err;

    {
        M_LOCK(n->parent->mx);
        n->src=src;
    }
    t.markDirty(n.get());

    t.gasUsed+=v->getGas("contract_update");

    b.emit_command(t.tx_id, seqId, "contract_update",R"({"name":"%s","owner":"%s"})",cn.container.c_str(),base16::encode(t.senderAddress.addr).c_str());

    return std::nullopt;
}
std::optional<std::string> TR::execute_vote(yyjson_val *params, b_params &b, t_params &t,
        int seqId)
{
    MUTEX_INSPECTOR;
    auto v = b.db->getValuesNoCreateConst();
    auto votes = b.db->getVotesOrCreate(t.roll);

    NODE_id node;
    auto err = yy_get_string(params, "node", node.container);
    if (err) return err;

    std::string param;
    err = yy_get_string(params, "param", param);
    if (err) return err;

    std::string value;
    err = yy_get_string(params, "value", value);
    if (err) return err;

    bool has_ttl = false;
    uint64_t ttl = 0;
    {
        auto tv = yyjson_obj_get(params, "ttl");
        if (tv) {
            if (yyjson_is_num(tv)) {
                ttl = yyjson_get_uint(tv);
            } else if (yyjson_is_str(tv)) {
                try { ttl = std::stoull(yyjson_get_str(tv)); }
                catch (...) { return "param 'ttl' invalid"; }
            } else {
                return "param 'ttl' must be number or string";
            }
            has_ttl = true;
        }
    }

    auto n = b.db->getNodeNoCreateConst(node);
    if (!n.valid())
        return "node not exists";
    if (n->get_owner() != t.senderAddress)
        return "insufficient privileges, node owner required";

    int64_t block_ts = b.validateBlockREQ->heart_beat->block_timestamp;
    auto vhash = blake2b_hash(param + value);

    {
        MutexLockerDeferred l(votes->parent->mx);
        l.lock();

        auto it = votes->votes.find(vhash);
        if (it == votes->votes.end()) {
            if (!has_ttl || ttl == 0) {
                l.unlock();
                return "param 'ttl' must be specified on first vote";
            }
            vote& z = votes->votes[vhash];
            z.param = param;
            z.value = std::stoull(value);
            z.create_time = block_ts;
            z.ttl = (int64_t)ttl;
            z.nodes.insert(node);
        } else {
            int64_t elapsed = block_ts - it->second.create_time;
            if (elapsed > it->second.ttl) {
                l.unlock();
                b.emit_command(t.tx_id, seqId, "vote_failed",
                    R"({"param":"%s","value":"%s","reason":"ttl_expired","elapsed":"%lld","ttl":"%lld"})",
                    param.c_str(), value.c_str(),
                    (long long)elapsed, (long long)it->second.ttl);
                {
                    MutexLockerDeferred l2(votes->parent->mx);
                    l2.lock();
                    votes->votes.erase(vhash);
                }
                t.markDirty(votes.get());
                return std::nullopt;
            }
            it->second.nodes.insert(node);
        }

        auto voted = votes->votes[vhash].nodes;
        l.unlock();
        t.markDirty(votes.get());

        uint64_t stake = 0;
        for (auto& z : voted) {
            auto nn = b.db->getNodeNoCreateConst(z);
            if (!nn.valid())
                return "node not found " + z.container;
            stake += nn->get_full_stake();
        }
        if ((stake * 100) / b.meta->fullStake() > QUORUM) {
            auto vv = b.db->getValuesNoCreate();
            {
                M_LOCK(vv->parent->mx);
                vv->fees[param] = std::stoull(value);
                if (param == "validator_min_stake")
                    vv->fees["validator_min_stake_activated"] = 1;   // активация sybil (getMetaFull — TBD)
            }
            votes->votes.erase(vhash);
            t.markDirty(votes.get());
            t.markDirty(vv.get());
            b.emit_command(t.tx_id, seqId, "vote_applied",
                R"({"param":"%s","value":"%s"})", param.c_str(), value.c_str());
        }
    }

    t.gasUsed += v->getGas("vote");
    b.emit_command(t.tx_id, seqId, "vote_counted",
        R"({"node":"%s","param":"%s","value":"%s"})",
        node.container.c_str(), param.c_str(), value.c_str());

    return std::nullopt;
}
