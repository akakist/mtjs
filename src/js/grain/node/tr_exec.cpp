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

std::optional<std::string> Node::Service::execute_mint(yyjson_val *params, b_params &b, t_params &t,
        int seqId)
{
    MUTEX_INSPECTOR;

    auto v = b.db->getValuesNoCreateConst();
    logNode("t.senderAddrss %s",base16::encode(t.senderAddress.addr).c_str());
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

 // ▼▼▼ INSERT: bc_state total_supply
    {
        auto st = b.db->getStateOrCreate(t.roll);
        if (!st.valid()) return "state create failed";
        { M_LOCK(st->parent->mx); st->total_supply += i128(amount); }
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

    auto u = b.db->getAddressStateNoCreateConst(t.senderAddress);
    if (!u.valid()) return "sender userstate invalid";
    if (to_addr == t.senderAddress) return "cannot transfer to self";

    auto to = b.db->getAddressStateNoCreate(to_addr);
    if (!to.valid()) return "execute_transfer: to address not found "+to_addr.str();

    if (amount > t.value) return "amount>t.value";

    t.value -= amount;
    {
        M_LOCK(to->parent->mx);
        to->balance += amount;
    }
    t.markDirty(to.get());

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


    t.gasUsed+=b.fees.gas_node_update;


    return std::nullopt;
}

std::optional<std::string> Node::Service::execute_node_create(yyjson_val *params, b_params &b, t_params &t,
        int seqId)
{
    MUTEX_INSPECTOR;
    auto v = b.db->getValuesNoCreateConst();
    NODE_id name;
    auto err=yy_get_string(params,"name",name.container);
    if(err) return err;
    for (auto &z : name.container)
    {
        if (!isalnum(z) && z!='_')
        {
            return "allowed only isalnum symbols and '_'";
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
    if(!n.valid())
        return "addNode failed";

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



    t.gasUsed+=b.fees.gas_node_create;

    // t.logMsg(txid, seqId, "node %s registered", name.container.c_str());
    b.emit_command(t.tx_id, seqId, "node_create",R"({"node":"%s","ip":"%s"})",name.container.c_str(),ip.c_str());

    return std::nullopt;
}
std::optional<std::string> Node::Service::execute_node_stake(yyjson_val *params, b_params & b,t_params &t,
        int seqId)
{
    MUTEX_INSPECTOR;
    auto v = b.db->getValuesNoCreateConst();

    int64_t amount=0;
    auto err=yy_get_int64_t(params,"amount",amount);
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

    t.gasUsed += b.fees.gas_node_stake;
  // ▼▼▼ INSERT: bc_state total_stake +=
    {
        auto st = b.db->getStateOrCreate(t.roll);
        if (!st.valid()) return "state create failed";
        { M_LOCK(st->parent->mx); st->total_stake += i128(amount); }
        t.markDirty(st.get());
    }
    // ▲▲▲    

    b.emit_command(t.tx_id, seqId, "node_stake",R"({"node":"%s","stake":"%s","from":"%s"})",
                   node.container.c_str(),
                   std::to_string(amount).c_str(),
                   base16::encode(t.senderAddress.addr).c_str()
                  );
    t.markDirty(n.get());


    return std::nullopt;
}
std::optional<std::string> Node::Service::execute_unstake_node(yyjson_val *params, b_params & b,t_params &t,
        int seqId)
{
    MUTEX_INSPECTOR;
    auto v = b.db->getValuesNoCreateConst();
    int64_t amount=0;
    auto err=yy_get_int64_t(params,"amount",amount);
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
    // ▼▼▼ INSERT: bc_state total_stake -=
    {
        auto st = b.db->getStateOrCreate(t.roll);
        if (!st.valid()) return "state create failed";
        { M_LOCK(st->parent->mx); st->total_stake -= i128(amount); }
        t.markDirty(st.get());
    }
    // ▲▲▲

    b.emit_command(t.tx_id, seqId, "node_unstake",R"({"node":"%s","from":"%s","amount":"%s"})",
                   node.container.c_str(),
                   base16::encode(t.senderAddress.addr).c_str(),
                   std::to_string(amount).c_str()
                  );
    return std::nullopt;
}

std::optional<std::string> Node::Service::execute_node_enable(yyjson_val *params, b_params & b,t_params &t,
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

    t.gasUsed+=b.fees.gas_node_enable;

    b.emit_command(t.tx_id, seqId, "node_enable",R"({"node":"%s"})", node.container.c_str());


    return std::nullopt;
}

std::optional<std::string> Node::Service::execute_node_disable(yyjson_val *params, b_params & b,t_params &t,
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

    t.gasUsed+=b.fees.gas_node_disable;

    b.emit_command(t.tx_id, seqId, "node_disable",R"({"node":"%s"})", node.container.c_str());



    return std::nullopt;
}



std::optional<std::string> Node::Service::execute_contract_deploy(yyjson_val *params, b_params &b, t_params &t,
        int seqId)
{
    MUTEX_INSPECTOR;
    auto v = b.db->getValuesNoCreateConst();
    std::string name;
    auto err=yy_get_string(params,"name",name);
    if(err) return err;
    for (auto &z : name)
    {
        if (!isalnum(z) && z!='_')
            return "allowed only isalnum symbols and '_'";
        if (isalpha(z) && isupper(z))
            return "allowed only lowercase symbols";
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
        n->owners.clear();
        n->owners.insert(t.senderAddress);   // создатель — owner
    }

    t.markDirty(n.get());
    // ▼▼▼ INSERT: bc_state contract_count++
    {
        auto st = b.db->getStateOrCreate(t.roll);
        if (!st.valid()) return "state create failed";
        { M_LOCK(st->parent->mx); st->contract_count++; }
        t.markDirty(st.get());
    }
    // ▲▲▲

    b.emit_command(t.tx_id, seqId, "contract_deploy",R"({"name":"%s","owner":"%s"})",name.c_str(),base16::encode(t.senderAddress.addr).c_str());

    return std::nullopt;
}


std::optional<std::string> Node::Service::execute_contract_update(yyjson_val *params, b_params &b, t_params &t,
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

    if (!n->isOwner(t.senderAddress))
        return "sender is not contract owner";

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


    b.emit_command(t.tx_id, seqId, "contract_update",R"({"name":"%s","owner":"%s"})",cn.container.c_str(),base16::encode(t.senderAddress.addr).c_str());

    contractHost->contract_cache.erase(cn);
    return std::nullopt;
}


std::optional<std::string> Node::Service::execute_vote(yyjson_val *params, b_params &b, t_params &t,
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

        int64_t stake = 0;
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


// TR::execute_gc(yyjson_val* params, b_params& b, t_params& t, int seqId)
// root (emitters_bin)
// params: { dbids: [hex, ...] }  (лимит — N, TBD)
// для каждого dbId → getByDbIdNoCreate → Cellable → data_base* → deleteGranule(data_base*, t.roll, db_dump)
// total_supply — не меняется (delete — TTL сгорел)
// emit gc { count }

std::optional<std::string> Node::Service::execute_gc(yyjson_val* params, b_params& b, t_params& t, int seqId, _db_to_save& dbd) 
{
    auto v = b.db->getValuesNoCreateConst();
    if (v->emitters_bin.find(t.senderAddress) == v->emitters_bin.end())
        return "insufficient_privileges gc";

    yyjson_val* arr = yyjson_obj_get(params, "dbids");
    if (!arr || !yyjson_is_arr(arr)) return "param 'dbids' must be array";
    yyjson_arr_iter it; yyjson_arr_iter_init(arr, &it);
    yyjson_val* item;
    int n = 0;
    _db_to_save db_dump;   // TBD — db_to_save_Z?
    while ((item = yyjson_arr_iter_next(&it))) {
        if (!yyjson_is_str(item)) return "dbid must be string";
        auto hex = std::string(yyjson_get_str(item));
        THASH_id h; h.container = base16::decode(hex);
        if (h.container.size() != 32) return "dbid must be 32 bytes hex";
        auto cc = b.db->getByDbIdNoCreate(h);
        if (!cc.valid()) continue;          // уже нет — skip
        if (!cc->data.valid()) continue;
        deleteGranule(cc->data.get(), t.roll, dbd);
        n++;
    }
    b.emit_command(t.tx_id, seqId, "gc", R"({"count":"%d"})", n);
    return std::nullopt;
}

std::optional<std::string> Node::Service::execute_create_address(yyjson_val* params, b_params& b, t_params& t, int seqId)
{
    std::string addr_hex;
    if (auto e = yy_get_string(params, "addr", addr_hex)) return e;

    auto addr_bin = base16::decode(addr_hex);
    if (addr_bin.size() != 32) return "addr: must be 32 bytes hex";

    ADDRESS_id addr; addr.addr = addr_bin;

    auto existing = b.db->getAddressStateNoCreate(addr);
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

    auto u = b.db->getAddressStateOrCreate(addr, t.roll);
    if (!u.valid()) return "create failed";
    for (auto& k : parsed) {
        auto id = u->addKey(k.type, k.kb);
        if (!id) return "addKey failed";
    }
    t.markDirty(u.get());
    t.gasUsed += b.fees.gas_create_address;
    b.emit_command(t.tx_id, seqId, "create_address",
        R"({"addr":"%s","keys":%d})", addr_hex.c_str(), (int)parsed.size());
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
    auto u = b.db->getAddressStateNoCreate(addr);
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
    auto u = b.db->getAddressStateNoCreate(addr);
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