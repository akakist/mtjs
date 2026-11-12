#include <fcntl.h>
#include <optional>
#include <yyjson.h>
#include <string>
#include "commonError.h"
#include "nodeService.h"
#include "ADDRESS_id.h"
#include "PK_id.h"
#include "yyjson_to_quickjs.h"
#include "QUORUM.h"
#include "normalizeNick.h"
#include "s_key.h"
#include "cells.h"

std::optional<std::string> Node::Service::execute_mint(yyjson_val *params, b_params &b, t_params &t,
        int seqId)
{
    MUTEX_INSPECTOR;

    auto v = cells::getValuesNoCreateConst(root.get(),db_state.get());
    auto it = v->emitters_bin.find(t.senderAddress);
    if (it == v->emitters_bin.end())
    {
        for(auto& z: v->emitters_bin)
        {
            logNode("emitter: %s",base16::encode(z.addr).c_str());
        }
        logErr2("insufficient_privileges emitter not found");
        return "insufficient_privileges";
    }
    int64_t amount=0;
    auto err=yy_get_int64_t(params,"amount",amount);
    if(err) return err;


    auto u = cells::getAddressStateNoCreate(root.get(), db_state.get(),t.senderAddress);
    if (!u.valid())
    {
        return "mint: sender not found";
    }
    {
        
        u->balance+=amount;
    }
    t.markDirty(u.get());
    cells::getActivityNoCreate(root.get(),db_state.get(),u->getDbId())->update_time(t.block_timestamp, t.roll);


 // ▼▼▼ INSERT: bc_state total_supply
    {
        auto st = cells::getStateNoCreate(root.get(), db_state.get());
        if (!st.valid()) return "state create failed";
        {  st->total_supply += i128(amount); }
        t.markDirty(st.get());
    }
    // ▲▲▲
    b.emit_command(t.tx_id, seqId,"mint",R"({"to":"%s","amount":"%s"})",
                   base16::encode(t.senderAddress.addr).c_str(),
                   std::to_string(amount).c_str());

    return std::nullopt;
}
std::optional<std::string> Node::Service::execute_transfer(yyjson_val *params, b_params &b, t_params &t, int seqId)
{
    MUTEX_INSPECTOR;

    int64_t amount = 0;
    if (auto err = yy_get_int64_t(params, "amount", amount)) return err;

    std::string to_hex;
    if (auto err = yy_get_string(params, "to", to_hex)) return err;

    ADDRESS_id to_addr;
    to_addr.addr = base16::decode(to_hex);
    if (to_addr.addr.size() != t.senderAddress.addr.size())
        return "param to has invalid size";

    auto u = cells::getAddressStateNoCreateConst(root.get(), db_state.get(),t.senderAddress);
    if (!u.valid()) return "sender userstate invalid";
    if (to_addr == t.senderAddress) return "cannot transfer to self";

    auto to = cells::getAddressStateNoCreate(root.get(), db_state.get(),to_addr);
    if (!to.valid()) return "execute_transfer: to address not found "+to_addr.str();

    if (amount > t.value) return "amount>t.value";

    t.value -= amount;
    {
        
        to->balance += amount;
    }
    t.markDirty(to.get());
    cells::getActivityNoCreate(root.get(),db_state.get(),to->getDbId())->update_time(t.block_timestamp, t.roll);

    b.emit_command(t.tx_id, seqId, "transfer",
        R"({"from":"%s","to":"%s","amount":"%s"})",
        base16::encode(t.senderAddress.addr).c_str(),
        base16::encode(to_addr.addr).c_str(),
        std::to_string(amount).c_str());
    return std::nullopt;
}


std::optional<std::string> Node::Service::execute_node_update(yyjson_val *params, b_params &b, t_params &t,
        int seqId)
{
    MUTEX_INSPECTOR;
    auto v = cells::getValuesNoCreateConst(root.get(),db_state.get());
    NODE_id name;
    auto err=yy_get_string(params,"name",name.container);
    if(err) return err;



    auto nn = cells::getNodeNoCreate(root.get(),db_state.get(), name);
    if (!nn.valid())
        return "Node not found";
    if(nn->get_owner()!=t.senderAddress)
    {
        return "only node owner can update node info";
    }
    auto us = cells::getAddressStateNoCreateConst(root.get(), db_state.get(),t.senderAddress);
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
    cells::getActivityNoCreate(root.get(),db_state.get(),nn->getDbId())->update_time(t.block_timestamp, t.roll);


    t.gasUsed+=b.fees.gas_node_update;


    return std::nullopt;
}

std::optional<std::string> Node::Service::execute_node_create(yyjson_val *params, b_params &b, t_params &t,
        int seqId)
{
    MUTEX_INSPECTOR;
    auto v = cells::getValuesNoCreateConst(root.get(),db_state.get());
    NODE_id name;
    auto err=yy_get_string(params,"name",name.container);
    if(err) return err;
    for (auto &z : name.container) {
        if (!isalnum(z) && z!='_') return "allowed only isalnum symbols and '_'";
        if (isalpha(z) && isupper(z)) return "allowed only lowercase symbols";
    }
    auto nn = cells::getNodeNoCreateConst(root.get(),db_state.get(),name);
    if (nn.valid()) return "Node already registered with name";

    auto us = cells::getAddressStateNoCreate(root.get(), db_state.get(),t.senderAddress);
    if (!us.valid()) return "if(!us.valid())";

    auto n = cells::addNode(root.get(),db_state.get(),name, t.roll);
    if(!n.valid()) return "addNode failed";

    std::string ip,pk_ed,pk_bls;
    int64_t ttl;
    err=yy_get_string(params,"ip",ip); if(err) return err;
    err=yy_get_string(params,"pk_ed",pk_ed); if(err) return err;
    err=yy_get_string(params,"pk_bls",pk_bls); if(err) return err;
    err=yy_get_int64_t(params,"ttl",ttl); if(err) return err;

    blst_cpp::PublicKey bls;
    bls.deserializeHexStr(pk_bls);
    n->init(name, t.senderAddress, bls, base16::decode(pk_ed), ip);

    // ▼▼▼ INSERT: обратный индекс owner -> owned_nodes
    us->owned_nodes.insert(name);
    t.markDirty(us.get());
    cells::getActivityNoCreate(root.get(),db_state.get(),us->getDbId())->update_time(t.block_timestamp,t.roll);
    

    // ▲▲▲

    t.markDirty(n.get());
    cells::getActivityOrCreate(root.get(),db_state.get(),n->getDbId(),{},ttl,t.block_timestamp,t.roll)->update_time(t.block_timestamp, t.roll);
    t.gasUsed+=b.fees.gas_node_create;

    b.emit_command(t.tx_id, seqId, "node_create",R"({"node":"%s","ip":"%s"})",name.container.c_str(),ip.c_str());
    return std::nullopt;
}


std::optional<std::string> Node::Service::execute_node_stake(yyjson_val *params, b_params & b,t_params &t,
        int seqId)
{
    MUTEX_INSPECTOR;
    auto v = cells::getValuesNoCreateConst(root.get(),db_state.get());

    int64_t amount=0;
    auto err=yy_get_int64_t(params,"amount",amount);
    if(err) return err;

    NODE_id node;
    err=yy_get_string(params,"node",node.container);
    if(err) return err;

    auto us = cells::getAddressStateNoCreate(root.get(), db_state.get(),t.senderAddress);
    if (!us.valid()) return "if(!us.valid())";

    auto n=cells::getNodeNoCreate(root.get(),db_state.get(),node);
    if(!n.valid()) return "node not found";
    if(t.value<amount) return "Insufficient funds";
    t.value-=amount;
    n->add_stake(t.senderAddress, amount);

    // ▼▼▼ INSERT: обратный индекс staker -> staked_in
    us->staked_in.insert(node);
    cells::getActivityNoCreate(root.get(),db_state.get(),us->getDbId())->update_time(t.block_timestamp, t.roll);
    t.markDirty(us.get());
    // ▲▲▲

    t.gasUsed += b.fees.gas_node_stake;
    {
        auto st = cells::getStateNoCreate(root.get(), db_state.get());
        if (!st.valid()) return "if (!st.valid())";
        st->total_stake += i128(amount);
        t.markDirty(st.get());
    }

    b.emit_command(t.tx_id, seqId, "node_stake",R"({"node":"%s","stake":"%s","from":"%s"})",
                   node.container.c_str(),
                   std::to_string(amount).c_str(),
                   base16::encode(t.senderAddress.addr).c_str());
    t.markDirty(n.get());
    cells::getActivityNoCreate(root.get(),db_state.get(),n->getDbId())->update_time(t.block_timestamp, t.roll);

    return std::nullopt;
}

std::optional<std::string> Node::Service::execute_unstake_node(yyjson_val *params, b_params & b,t_params &t,
        int seqId)
{
    MUTEX_INSPECTOR;
    auto v = cells::getValuesNoCreateConst(root.get(),db_state.get());
    int64_t amount=0;
    auto err=yy_get_int64_t(params,"amount",amount);
    if(err) return err;
    NODE_id node;
    err=yy_get_string(params,"node",node.container);
    if(err) return err;

    auto n = cells::getNodeNoCreate(root.get(),db_state.get(),node);
    if (!n.valid()) return "nodes not registered";
    auto stake=n->get_user_stake(t.senderAddress);
    if (stake == 0) return "no_stake_found";
    if (stake < amount) return "insufficient stake in node";

    auto u = cells::getAddressStateNoCreate(root.get(), db_state.get(),t.senderAddress);
    if (!u.valid()) return "if(!u.valid())";
    t.value+=amount;
    n->sub_stake(t.senderAddress, amount);

    // ▼▼▼ INSERT: если весь stake выведен — staked_in.erase
    if (n->get_user_stake(t.senderAddress) == 0) {
        u->staked_in.erase(node);
    }
    auto actu=cells::getActivityNoCreate(root.get(),db_state.get(),u->getDbId());
    actu->update_time(t.block_timestamp,t.roll);
    auto actn=cells::getActivityNoCreate(root.get(),db_state.get(),n->getDbId());
    actn->update_time(t.block_timestamp,t.roll);

    t.markDirty(u.get());

    t.markDirty(n.get());
    cells::getActivityNoCreate(root.get(),db_state.get(),n->getDbId())->update_time(t.block_timestamp, t.roll);
    cells::getActivityNoCreate(root.get(),db_state.get(),u->getDbId())->update_time(t.block_timestamp, t.roll);
    {
        auto st = cells::getStateNoCreate(root.get(), db_state.get());
        if (!st.valid()) return "state create failed";
        st->total_stake -= i128(amount);
        t.markDirty(st.get());
    }

    b.emit_command(t.tx_id, seqId, "node_unstake",R"({"node":"%s","from":"%s","amount":"%s"})",
                   node.container.c_str(),
                   base16::encode(t.senderAddress.addr).c_str(),
                   std::to_string(amount).c_str());
    return std::nullopt;
}

std::optional<std::string> Node::Service::execute_node_enable(yyjson_val *params, b_params & b,t_params &t,
        int seqId)
{
    MUTEX_INSPECTOR;
    auto v = cells::getValuesNoCreateConst(root.get(),db_state.get());
    NODE_id node;
    auto err=yy_get_string(params,"node",node.container);
    if(err)return err;

    auto n = cells::getNodeNoCreate(root.get(),db_state.get(),node);
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
    cells::getActivityNoCreate(root.get(),db_state.get(),n->getDbId())->update_time(t.block_timestamp, t.roll);

    t.gasUsed+=b.fees.gas_node_enable;

    b.emit_command(t.tx_id, seqId, "node_enable",R"({"node":"%s"})", node.container.c_str());


    return std::nullopt;
}

std::optional<std::string> Node::Service::execute_node_disable(yyjson_val *params, b_params & b,t_params &t,
        int seqId)
{
    MUTEX_INSPECTOR;
    auto v = cells::getValuesNoCreateConst(root.get(),db_state.get());
    NODE_id node;
    auto err=yy_get_string(params,"node",node.container);
    if(err)return err;

    auto n = cells::getNodeNoCreate(root.get(),db_state.get(),node);
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
    cells::getActivityNoCreate(root.get(),db_state.get(),n->getDbId())->update_time(t.block_timestamp, t.roll);
    

    t.gasUsed+=b.fees.gas_node_disable;

    b.emit_command(t.tx_id, seqId, "node_disable",R"({"node":"%s"})", node.container.c_str());



    return std::nullopt;
}



std::optional<std::string> Node::Service::execute_contract_deploy(yyjson_val *params, b_params &b, t_params &t,
        int seqId)
{
    MUTEX_INSPECTOR;
    std::string name;
    auto err = yy_get_string(params, "name", name);
    if (err) return err;
    for (auto &z : name)
    {
        if (!isalnum(z) && z != '_')
            return "allowed only isalnum symbols and '_'";
        if (isalpha(z) && isupper(z))
            return "allowed only lowercase symbols";
    }
    CONTRACT_id cn;
    cn.container = name;
    auto nn = cells::getContractNoCreateConst(root.get(), db_state.get(), cn);
    if (nn.valid())
        return "Contract already registered with name";

    auto us = cells::getAddressStateNoCreate(root.get(), db_state.get(), t.senderAddress);
    if (!us.valid())
        return "if(!us.valid())";

    // ttl обязателен
    int64_t ttl = 0;
    if (auto e = yy_get_int64_t(params, "ttl", ttl)) return e;
    if (ttl <= 0) return "contract_deploy: ttl must be > 0";
    if (ttl > MAX_GRANULE_TTL) return "contract_deploy: ttl > MAX_TTL";

    auto n = cells::getContractOrCreate(root.get(), db_state.get(), cn, t.roll);

    std::string src;
    err = yy_get_string(params, "src", src);
    if (err) return err;

    {
        n->src = src;
        n->owners.clear();
        n->owners.insert(t.senderAddress);   // создатель — owner
    }
    t.markDirty(n.get());

    auto ab = cells::getContractBalanceOrCreate(root.get(), db_state.get(), cn, t.roll);
    if (!ab.valid())
        throw CommonError("if(!ab.valid())");
    ab->setDirty(t.roll);

    // ▼▼▼ корневая гранула контракта (dataId="")
    CONTRACT_DATA_id rootKey;
    rootKey.container = "";
    auto rootGranule = cells::getContractDataOrCreate(root.get(), db_state.get(), cn, rootKey,
                                                      t.roll, ttl, t.block_timestamp);
    if (!rootGranule.valid())
        return "contract_deploy: root granule create failed";
    rootGranule->container = "";   // пустой value
    t.markDirty(rootGranule.get());

    // activity для корневой гранулы (delete_list = root_granule)
    auto rootAct = cells::getActivityOrCreate(root.get(), db_state.get(), rootGranule->getDbId(),
                                              {}, ttl, t.block_timestamp, t.roll);
    if (rootAct.valid()) {
        rootAct->dbid = { base16::encode(rootGranule->getDbId()) };
        rootAct->update_time(t.block_timestamp, t.roll);
    }
    // ▲▲▲

    // activity контракта (delete_list = contract + balance + root_granule)
    auto act = cells::getActivityOrCreate(root.get(), db_state.get(), n->getDbId(), {}, ttl, t.block_timestamp, t.roll);
    if (act.valid()) {
        act->dbid = {
            n->getDbId(),           // contract
            ab->getDbId(),          // balance
            rootGranule->getDbId(), // root granule
        };
        act->update_time(t.block_timestamp, t.roll);
    }

    // газ: gas_contract_deploy + ceil(ttl/ttl_seconds_per_gas)
    int64_t ttl_gas = b.fees.gas_contract_deploy
                    + (ttl + b.fees.ttl_seconds_per_gas - 1) / b.fees.ttl_seconds_per_gas;
    t.gasUsed += ttl_gas;

    // bc_state contract_count++
    {
        auto st = cells::getStateNoCreate(root.get(), db_state.get());
        if (!st.valid()) return "state create failed";
        st->contract_count++;
        t.markDirty(st.get());
    }

    us->contracts.insert(cn);
    t.markDirty(us.get());
    cells::getActivityNoCreate(root.get(), db_state.get(), us->getDbId())->update_time(t.block_timestamp, t.roll);

    b.emit_command(t.tx_id, seqId, "contract_deploy",
        R"({"name":"%s","owner":"%s","ttl":"%lld"})",
        name.c_str(), base16::encode(t.senderAddress.addr).c_str(), (long long)ttl);

    return std::nullopt;
}

std::optional<std::string> Node::Service::execute_contract_update(yyjson_val *params, b_params &b, t_params &t,
        int seqId)
{
    MUTEX_INSPECTOR;
    auto v = cells::getValuesNoCreateConst(root.get(),db_state.get());
    CONTRACT_id cn;
    auto err=yy_get_string(params,"name",cn.container);
    if(err)
        return err;

    auto n = cells::getContractNoCreate(root.get(),db_state.get(),cn);
    if (!n.valid())
        return "contract not exists";

    if (!n->isOwner(t.senderAddress))
        return "sender is not contract owner";

    auto us = cells::getAddressStateNoCreateConst(root.get(), db_state.get(),t.senderAddress);
    if (!us.valid())
        return "if(!us.valid())";

    std::string src;
    err=yy_get_string(params,"src",src);
    if(err) return err;

    {
        
        n->src=src;
    }
    t.markDirty(n.get());
    cells::getActivityNoCreate(root.get(),db_state.get(),us->getDbId())->update_time(t.block_timestamp,t.roll);
    cells::getActivityNoCreate(root.get(),db_state.get(),n->getDbId())->update_time(t.block_timestamp,t.roll);


    b.emit_command(t.tx_id, seqId, "contract_update",R"({"name":"%s","owner":"%s"})",cn.container.c_str(),base16::encode(t.senderAddress.addr).c_str());

    contractHost->contract_cache.erase(cn);
    return std::nullopt;
}


std::optional<std::string> Node::Service::execute_vote(yyjson_val *params, b_params &b, t_params &t,
        int seqId)
{
    MUTEX_INSPECTOR;
    auto v = cells::getValuesNoCreateConst(root.get(),db_state.get());
    auto votes = cells::getVotesNoCreate(root.get(),db_state.get());

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
    int64_t ttl = 0;
    {
        auto tv = yyjson_obj_get(params, "ttl");
        if (tv) {
            if (yyjson_is_num(tv)) {
                ttl = yyjson_get_sint(tv);
            } else if (yyjson_is_str(tv)) {
                try { ttl = std::stoull(yyjson_get_str(tv)); }
                catch (...) { return "param 'ttl' invalid"; }
            } else {
                return "param 'ttl' must be number or string";
            }
            has_ttl = true;
        }
    }

    auto n = cells::getNodeNoCreateConst(root.get(),db_state.get(),node);
    if (!n.valid())
        return "node not exists";
    if (n->get_owner() != t.senderAddress)
        return "insufficient privileges, node owner required";

    int64_t block_ts = b.validateBlockREQ->heart_beat->block_timestamp;
    auto vhash = blake2b_hash(param + value);

    {

        auto it = votes->votes.find(vhash);
        if (it == votes->votes.end()) {
            if (!has_ttl || ttl == 0) {
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
                b.emit_command(t.tx_id, seqId, "vote_failed",
                    R"({"param":"%s","value":"%s","reason":"ttl_expired","elapsed":"%lld","ttl":"%lld"})",
                    param.c_str(), value.c_str(),
                    (long long)elapsed, (long long)it->second.ttl);
                {
                    votes->votes.erase(vhash);
                }
                t.markDirty(votes.get());

                return std::nullopt;
            }
            it->second.nodes.insert(node);
        }

        auto voted = votes->votes[vhash].nodes;
        t.markDirty(votes.get());

        int64_t stake = 0;
        for (auto& z : voted) {
            auto nn = cells::getNodeNoCreateConst(root.get(),db_state.get(),z);
            if (!nn.valid())
                return "node not found " + z.container;
            stake += nn->get_full_stake();
        }
        if ((stake * 100) / b.meta->fullStake() > QUORUM) {
            auto vv = cells::getValuesNoCreate(root.get(),db_state.get());
            {
                
                vv->values[param] = std::stoull(value);
                if (param == "validator_min_stake")
                    vv->values["validator_min_stake_activated"] = 1;   // активация sybil (getMetaFull — TBD)
            }
            votes->votes.erase(vhash);
            t.markDirty(votes.get());
            t.markDirty(vv.get());
            b.emit_command(t.tx_id, seqId, "vote_applied",
                R"({"param":"%s","value":"%s"})", param.c_str(), value.c_str());
        }
    }

    b.emit_command(t.tx_id, seqId, "vote_counted",
        R"({"node":"%s","param":"%s","value":"%s"})",
        node.container.c_str(), param.c_str(), value.c_str());

    return std::nullopt;
}

std::optional<std::string> Node::Service::execute_gc(
    yyjson_val* params, b_params& b, t_params& t, int seqId, _db_to_save& dbd)
{
    logNode("execute_gc: START items=%d", (int)yyjson_arr_size(yyjson_obj_get(params, "items")));
    auto v = cells::getValuesNoCreateConst(root.get(), db_state.get());
    if (!v.valid()) return "values not found";
    if (v->emitters_bin.find(t.senderAddress) == v->emitters_bin.end())
        return "insufficient_privileges gc";

    yyjson_val* arr = yyjson_obj_get(params, "items");
    if (!arr || !yyjson_is_arr(arr)) return "param 'items' must be array";

    // ← P0: now = max(block_timestamp, time(NULL))
    // (scan_expired использует time(NULL); gc не должен отставать из-за block_timestamp prev_block)
    int64_t now = std::max((int64_t)t.block_timestamp, (int64_t)time(NULL));

    int applied = 0, skipped = 0;
    int applied_granule = 0, applied_contract = 0, applied_node = 0, applied_address = 0;

    yyjson_arr_iter it; yyjson_arr_iter_init(arr, &it);
    yyjson_val* item;
    while ((item = yyjson_arr_iter_next(&it))) {
        if (!yyjson_is_str(item)) { skipped++; continue; }
        auto hex = std::string(yyjson_get_str(item));
        THASH_id h; h.container = base16::decode(hex);
        if (h.container.size() != 32) { skipped++; continue; }

        auto cc = cells::getByDbIdNoCreate(root.get(), db_state.get(), h);
        if (!cc.valid() || !cc->data.valid()) { skipped++; continue; }

        logNode("execute_gc: dbid=%s payload_ctor_idx=%d", hex.c_str(), (int)cc->payload_ctor_idx);

        std::optional<std::string> err;
        switch (cc->payload_ctor_idx) {
        case hsh::bc_activity:
            err = gc_activity_apply(cc, now, t, b, dbd, applied_granule, applied_contract, applied_node, applied_address);
            // ← P0-3: applied_contract++ убран (gc_activity_apply уже считает по payload_ctor_idx)
            break;
        default:
            err = "not_gc_type";
            break;
        }

        if (err) skipped++;
        else     applied++;
    }

    b.emit_command(t.tx_id, seqId, "gc_granule",  R"({"count":%d})", applied_granule);
    b.emit_command(t.tx_id, seqId, "gc_contract", R"({"count":%d})", applied_contract);
    b.emit_command(t.tx_id, seqId, "gc_node",     R"({"count":%d})", applied_node);
    b.emit_command(t.tx_id, seqId, "gc_address",  R"({"count":%d})", applied_address);
    b.emit_command(t.tx_id, seqId, "gc",          R"({"applied":%d,"skipped":%d})", applied, skipped);
    logNode("execute_gc: applied=%d skipped=%d granule=%d", applied, skipped, applied_granule);
    return std::nullopt;
}
std::optional<std::string> Node::Service::execute_add_address_key(yyjson_val* params, b_params& b, t_params& t, int seqId)
{
    std::string addr_hex; int64_t type = 0; std::string kb_hex, sig_hex;
    if (auto e = yy_get_string(params, "addr", addr_hex)) return e;
    if (auto e = yy_get_int64_t(params, "type", type)) return e;
    if (auto e = yy_get_string(params, "keybuffer", kb_hex)) return e;
    if (auto e = yy_get_string(params, "sig", sig_hex)) return e;

    ADDRESS_id addr; addr.addr = base16::decode(addr_hex);
    auto u = cells::getAddressStateNoCreate(root.get(), db_state.get(),addr);
    if (!u.valid()) return "execute_add_address_key: address not found";

    bool is_owner = false;
    for (auto& [id, k] : u->keys) if (k.keybuffer == t.tx->pk_ed_bin) { is_owner = true; break; }
    if (!is_owner) return "sender is not an owner";

    auto kb = base16::decode(kb_hex);
    if (type == KEY_ED25519 && kb.size() != 32) return "key: bad ed25519 size";
    if ((type == KEY_SECP256K1 || type == KEY_P256) && kb.size() != 33 && kb.size() != 65)
        return "key: bad ecdsa size (33/65)";

    std::string msg = "add_address_key:" + addr_hex + ":" + kb_hex;
    if (!verify(Key{0, (uint8_t)type, kb}, base16::decode(sig_hex), msg))
        return "sig: invalid";

    auto id = u->addKey((uint8_t)type, kb);
    if (!id) return "too many keys (max 5)";
    t.markDirty(u.get());
    b.emit_command(t.tx_id, seqId, "add_address_key",
        R"({"addr":"%s","key_id":%llu})", addr_hex.c_str(), (unsigned long long)*id);
    return std::nullopt;
}

std::optional<std::string> Node::Service::execute_remove_address_key(yyjson_val* params, b_params& b, t_params& t, int seqId)
{
    std::string addr_hex; int64_t key_id = 0;
    if (auto e = yy_get_string(params, "addr", addr_hex)) return e;
    if (auto e = yy_get_int64_t(params, "key_id", key_id)) return e;

    ADDRESS_id addr; addr.addr = base16::decode(addr_hex);
    auto u = cells::getAddressStateNoCreate(root.get(), db_state.get(),addr);
    if (!u.valid()) return (std::string)"execute_remove_address_key: address not found "+addr.str();

    bool is_owner = false;
    for (auto& [id, k] : u->keys) if (k.keybuffer == t.tx->pk_ed_bin) { is_owner = true; break; }
    if (!is_owner) return "sender is not an owner";

    if (!u->hasKeyId((uint64_t)key_id)) return "key_id not found";
    if (u->sizeKeys() <= 1) return "cannot remove last key";

    u->removeKey((uint64_t)key_id);
    t.markDirty(u.get());
    b.emit_command(t.tx_id, seqId, "remove_address_key",
        R"({"addr":"%s","key_id":%lld})", addr_hex.c_str(), (long long)key_id);
    return std::nullopt;
}



std::optional<std::string> Node::Service::execute_create_address(yyjson_val* params, b_params& b, t_params& t, int seqId)
{
    std::string addr_hex;
    int64_t ttl;
    if (auto e = yy_get_string(params, "addr", addr_hex)) return e;
    if (auto e = yy_get_int64_t(params, "ttl", ttl)) return e;

    auto addr_bin = base16::decode(addr_hex);
    if (addr_bin.size() != 32) return "addr: must be 32 bytes hex";

    ADDRESS_id addr; addr.addr = addr_bin;

    auto existing = cells::getAddressStateNoCreate(root.get(), db_state.get(),addr);
    if (existing.valid()) return "address already exists";

    yyjson_val* keys_arr = yyjson_obj_get(params, "keys");
    if (!keys_arr || !yyjson_is_arr(keys_arr)) return "keys: must be array";
    size_t nkeys = yyjson_arr_size(keys_arr);
    if (nkeys == 0 || nkeys > MAX_KEYS_PER_ADDRESS) return "keys: 1..5 required";

    struct K { uint8_t type; std::string kb; };
    std::vector<K> parsed;
    yyjson_arr_iter it; yyjson_arr_iter_init(keys_arr, &it);
    yyjson_val* item;
    while ((item = yyjson_arr_iter_next(&it))) {
        int64_t type = 0; std::string kb_hex;
        if (auto e = yy_get_int64_t(item, "type", type)) return e;
        if (auto e = yy_get_string(item, "keybuffer", kb_hex)) return e;
        auto kb = base16::decode(kb_hex);
        if (type == KEY_ED25519 && kb.size() != 32) return "key: bad ed25519 size";
        if ((type == KEY_SECP256K1 || type == KEY_P256) && kb.size() != 33 && kb.size() != 65)
            return "key: bad ecdsa size (33/65)";
        parsed.push_back({(uint8_t)type, kb});
    }

    auto u = cells::getAddressStateOrCreate(root.get(), db_state.get(),addr, t.roll);
    if (!u.valid()) return "create failed";
    for (auto& k : parsed) {
        auto id = u->addKey(k.type, k.kb);
        if (!id) return "addKey failed";
    }
    t.markDirty(u.get());
    cells::getActivityOrCreate(root.get(),db_state.get(),u->getDbId(),{},ttl,t.block_timestamp,t.roll)->update_time(t.block_timestamp,t.roll);

    t.gasUsed += b.fees.gas_create_address;
    b.emit_command(t.tx_id, seqId, "create_address",
        R"({"addr":"%s","keys":%d})", addr_hex.c_str(), (int)parsed.size());
    return std::nullopt;
}



//////////////////// GC HELPERS


// std::optional<std::string> Node::Service::gc_activity_apply(
//     const REF_getter<Cellable>& cc, int64_t now, t_params& t, b_params& b, _db_to_save& dbd)
std::optional<std::string> Node::Service::gc_activity_apply(
        const REF_getter<Cellable>& cc, int64_t now, t_params& t, b_params& b, _db_to_save& dbd,
        int& applied_granule, int& applied_contract, int& applied_node, int& applied_address)
{
    if (!cc->data.valid()) return "no data";
    auto* act = dynamic_cast<bc_activity*>(cc->data.get());
    if (!act) return "not bc_activity";
    if (act->ttl <= 0) return "ttl <= 0";
    if (act->_update_time + act->ttl >= now) return "not expired";

    for (auto& dbid : act->dbid) {
        THASH_id h;
        h.container = dbid;   // ← bin (32 байта), не hex (64)
        if (h.container.size() != 32) {
            logNode("gc: dbid '%s' bad size (skip)", dbid.c_str());
            continue;
        }

        auto g = cells::getByDbIdNoCreate(root.get(), db_state.get(), h);
        if (!g.valid() || !g->data.valid()) {
            logNode("gc: dbid %s not found (skip)", dbid.c_str());
            continue;
        }

        logNode("gc: dbid=%s ctor_idx=%d", base16::encode(dbid).c_str(), (int)g->payload_ctor_idx);

        switch (g->payload_ctor_idx) {
        case hsh::bc_granule:
            applied_granule++;
            deleteGranule(g->data.get(), t.roll, dbd);
            break;

        case hsh::bc_node: {
            applied_node++;
            auto* n = dynamic_cast<bc_node*>(g->data.get());
            if (!n) break;
            if (n->isEnabled()) {
                logNode("gc: node %s enabled (skip)", n->getName().container.c_str());
                break;
            }
            auto name = n->getName();

            // 1. Вернуть stakes стейкерам
            auto st = cells::getStateNoCreate(root.get(), db_state.get());
            if (!st.valid()) break;
            for (auto& [user_addr, stake] : n->getStakes()) {
                auto u = cells::getAddressStateNoCreate(root.get(), db_state.get(), user_addr);
                if (u.valid()) {
                    u->balance += stake;
                    u->staked_in.erase(name);
                    t.markDirty(u.get());
                    cells::getActivityNoCreate(root.get(), db_state.get(), u->getDbId())
                        ->update_time(t.block_timestamp, t.roll);
                }
                st->total_stake -= i128(stake);
            }
            t.markDirty(st.get());

            // 2. Удалить из bc_nodelist
            auto nl = cells::getNodeListNoCreate(root.get(), db_state.get());
            if (nl.valid()) {
                nl->remove(name);
                t.markDirty(nl.get());
            }

            // 3. Удалить bc_node
            n->setDirty(t.roll);
            deleteGranule(n, t.roll, dbd);

            b.emit_command(t.tx_id, 0, "gc_node",
                R"({"name":"%s","stake_returned":true})", name.container.c_str());
            break;
        }

        case hsh::bc_address_state: {
            auto* u = dynamic_cast<bc_address_state*>(g->data.get());
            if (!u) break;
            if (!u->contracts.empty()) { logNode("gc: address has contracts (skip)"); break; }
            if (!u->staked_in.empty()) { logNode("gc: address is staker (skip)"); break; }

            applied_address++;
            int64_t burned = u->balance;
            auto st = cells::getStateNoCreate(root.get(), db_state.get());
            if (st.valid() && burned > 0) {
                st->total_supply -= i128(burned);
                t.markDirty(st.get());
            }

            deleteGranule(u, t.roll, dbd);
            b.emit_command(t.tx_id, 0, "gc_address_deleted",
                R"({"addr":"%s","burned":"%lld"})",
                base16::encode(g->getDbId()).c_str(), (long long)burned);
            break;
        }

        case hsh::bc_contract: {
            applied_contract++;
            auto* c = dynamic_cast<bc_contract*>(g->data.get());
            if (!c) break;
            auto name = c->name_;

            for (auto& owner_addr : c->owners) {
                auto u = cells::getAddressStateNoCreate(root.get(), db_state.get(), owner_addr);
                if (u.valid()) {
                    CONTRACT_id cn; cn.container = name;
                    u->contracts.erase(cn);
                    t.markDirty(u.get());
                    cells::getActivityNoCreate(root.get(), db_state.get(), u->getDbId())
                        ->update_time(t.block_timestamp, t.roll);
                }
            }
            c->owners.clear();

            deleteGranule(c, t.roll, dbd);
            b.emit_command(t.tx_id, 0, "gc_contract_deleted",
                R"({"name":"%s"})", name.c_str());
            break;
        }

        case hsh::bc_contract_balance: {
            auto* bal = dynamic_cast<bc_contract_balance*>(g->data.get());
            if (bal) {
                auto st = cells::getStateNoCreate(root.get(), db_state.get());
                if (st.valid() && bal->getAmount() > 0) {
                    st->total_supply -= i128(bal->getAmount());
                    t.markDirty(st.get());
                }
            }
            deleteGranule(g->data.get(), t.roll, dbd);
            break;
        }

        default:
            deleteGranule(g->data.get(), t.roll, dbd);
            break;
        }
    }
    deleteGranule(act, t.roll, dbd);
    return std::nullopt;
}