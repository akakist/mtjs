#include "cells.h"
#include "IDatabase.h"
#include "normalizeNick.h"
#include "blake2bHasher.h"
#include "commonError.h"

namespace cells {

// ============================================================
// path computation (pure)
// ============================================================
    std::vector<std::string> getPathFromHash(const THASH_id& h)
    {
        std::vector<std::string> out;
        out.reserve(5);
        out.emplace_back(h.container, 0, 1);
        out.emplace_back(h.container, 1, 1);
        out.emplace_back(h.container, 2, 1);
        out.emplace_back(h.container, 3, 1);
        out.emplace_back(h.container, 4, 28);
        return out;
    }

    std::vector<std::string> getPath(const std::string& name)
    {
        auto h = blake2b_hash(name);
        return getPathFromHash(h);
    }

    std::vector<std::string> getAddressStatePath(const ADDRESS_id& addr)
    {
        MUTEX_INSPECTOR;
        if (addr.addr.empty())
            throw CommonError("getAddressStatePath: empty addr");
        return getPath("USER_STATE_" + addr.addr);
    }

    std::vector<std::string> getContractPath(const CONTRACT_id& name)
    {
        return getPath("CONTRACT_" + name.container);
    }

    std::vector<std::string> getContractDataPath(const CONTRACT_id& contract, const CONTRACT_DATA_id& dataId)
    {
        std::string key = "CONTRACT_DATA_" + contract.container + "_" + dataId.container;
        return getPath(key);
    }

    std::vector<std::string> getNodePath(const NODE_id& name)
    {
        return getPath("NODE_" + name.container);
    }

    std::vector<std::string> getContractBalancePath(const CONTRACT_id& contract)
    {
        return getPath("CONTRACT_BALANCE_" + contract.container);
    }

    std::vector<std::string> getStatePath()
    {
        return getPath("STATE");
    }

// ============================================================
// replaceLeaf / getByPath / getByDbId
// ============================================================
    REF_getter<Cellable> replaceLeafOrCreate(Cellable* _cur, IDatabase* db, const std::string& id, Rollback* roll)
    {
        MUTEX_INSPECTOR;
        auto cur = _cur;
        auto it = cur->children_hashes_mx.find(id);
        if (it != cur->children_hashes_mx.end())
        {
            return replaceLeafNoCreate(cur, db, id);
        }
        if (roll)
        {
            if (!roll->data.count(cur))
            {
                roll->data[cur] = cur->getBuffer_mx();
            }
        }
        cur->children_hashes_mx[id].container = "";
        auto it2 = cur->children_ptrs_mx.find(id);
        if (it2 != cur->children_ptrs_mx.end())
            throw CommonError("if(it!=children_ptrs.end())");
        REF_getter<Cellable> c = new Cellable(cur, id);
        cur->children_ptrs_mx.insert({id, c});
        c->setDirty__(roll);
        return c;
    }

    REF_getter<Cellable> replaceLeafNoCreate(Cellable* _cur, IDatabase* db, const std::string& id)
    {
        MUTEX_INSPECTOR;
        auto cur = _cur;
        {
        }
        auto ip = cur->children_ptrs_mx.find(id);
        if (ip != cur->children_ptrs_mx.end())
        {
            return ip->second;
        }
        auto it = cur->children_hashes_mx.find(id);
        if (it == cur->children_hashes_mx.end())
        {
            return NULL;
        }
        REF_getter<Cellable> cc = new Cellable(cur, id);

        std::string result;
        auto dbId = cc->getDbId();
        int r = db->getGranule(dbId, &result);
        if (r)
            logErr2("db->getGranule err %s", base16::encode(cc->getDbId()).c_str());

        if (result.size())
        {
            auto h = blake2b_hash(result);
            if (it->second == h)
            {
                inBuffer in(result);
                cc->unpack_mx(in);
            }
            else
            {
                db->clear_root = true;
                db->add_sync_out(dbId);
                throw CommonError("getGranule: cell hash not matched %s %s granule size %ld body %s",
                                  base16::encode(it->second.container).c_str(),
                                  base16::encode(h.container).c_str(),
                                  result.size(),
                                  base16::encode(result).c_str());
            }
        }
        cur->children_ptrs_mx.insert({id, cc});
        return cc;
    }

    REF_getter<Cellable> getByPathOrCreate(Cellable* root, IDatabase* db, const std::vector<std::string>& v, Rollback* roll)
    {
        auto cur = root;
        for (auto& z : v)
        {
            cur = replaceLeafOrCreate(cur, db, z,  roll).get();
        }
        return cur;
    }

    REF_getter<Cellable> getByPathOrCreate(Cellable* root, IDatabase* db, const std::deque<std::string>& v, Rollback* roll)
    {
        auto cur = root;
        for (auto& z : v)
        {
            cur = replaceLeafOrCreate(cur, db, z,  roll).get();
        }
        return cur;
    }

    REF_getter<Cellable> getByPathNoCreate(Cellable* root, IDatabase* db, const std::vector<std::string>& v)
    {
        auto cur = root;
        for (auto& z : v)
        {
            auto next = replaceLeafNoCreate(cur, db, z);
            if (!next.valid())
                return NULL;
            cur = next.get();
        }
        return cur;
    }

    REF_getter<Cellable> getByDbIdNoCreate(Cellable* root, IDatabase* db, const THASH_id& h)
    {
        MUTEX_INSPECTOR;
        return getByPathNoCreate(root, db, getPathFromHash(h));
    }

    REF_getter<data_base> replaceLeaf(Cellable* root, IDatabase* db, const THASH_id& h, Rollback* roll, const std::string& buffer)
    {
        MUTEX_INSPECTOR;
        auto cc = getByPathOrCreate(root, db, getPathFromHash(h), roll);
        if (!cc.valid())
            throw CommonError("if (!cc.valid()) 4");
        {
            
            inBuffer in(buffer);
            cc->unpack_mx(in);
        }
        if (!cc->data.valid())
            throw CommonError("if(!cc->data.valid())");
        cc->data->setDirty(roll);
        return cc->data;
    }

// ============================================================
// address state
// ============================================================
    REF_getter<bc_address_state> getAddressStateOrCreate(Cellable* root, IDatabase* db, const ADDRESS_id& addr, Rollback* roll)
    {
        MUTEX_INSPECTOR;
        auto cc = getByPathOrCreate(root, db, getAddressStatePath(addr), roll);
        if (!cc.valid())
            return NULL;
        if (cc->data.valid())
            return dynamic_cast<bc_address_state*>(cc->data.get());

        REF_getter<bc_address_state> u = new bc_address_state(cc.get());
        cc->data = u.get();
        cc->payload_ctor_idx = hsh::bc_address_state;
        {
            auto st = getStateNoCreate(root, db);
            if (st.valid())
            {
                
                st->account_count++;
            }
            st->setDirty(roll);
        }
        return u;
    }

    REF_getter<bc_address_state> getAddressStateNoCreate(Cellable* root, IDatabase* db, const ADDRESS_id& addr)
    {
        MUTEX_INSPECTOR;
        auto cc = getByPathNoCreate(root, db, getAddressStatePath(addr));
        if (!cc.valid())
            return NULL;
        if (!cc->data.valid())
            return NULL;
        return dynamic_cast<bc_address_state*>(cc->data.get());
    }

    REF_getter<const bc_address_state> getAddressStateNoCreateConst(Cellable* root, IDatabase* db, const ADDRESS_id& addr)
    {
        MUTEX_INSPECTOR;
        auto cc = getByPathNoCreate(root, db, getAddressStatePath(addr));
        if (!cc.valid())
            return NULL;
        if (!cc->data.valid())
            return NULL;
        return dynamic_cast<const bc_address_state*>(cc->data.get());
    }

// ============================================================
// contract / contract data / contract balance
// ============================================================
    REF_getter<bc_contract> getContractOrCreate(Cellable* root, IDatabase* db, const CONTRACT_id& name, Rollback* roll)
    {
        MUTEX_INSPECTOR;
        auto cc = getByPathOrCreate(root, db, getContractPath(name), roll);
        if (!cc.valid())
            throw CommonError("if(!cc.valid())");
        if (cc->data.valid())
            throw CommonError("if(cc->data.valid())");
        REF_getter<bc_contract> bc = new bc_contract(cc.get());
        cc->data = bc.get();
        cc->payload_ctor_idx = hsh::bc_contract;
        cc->data->setDirty(roll);
        return bc;
    }

    REF_getter<bc_contract> getContractNoCreate(Cellable* root, IDatabase* db, const CONTRACT_id& name)
    {
        MUTEX_INSPECTOR;
        auto cc = getByPathNoCreate(root, db, getContractPath(name));
        if (!cc.valid())
            return NULL;
        if (!cc->data.valid())
            return NULL;
        return dynamic_cast<bc_contract*>(cc->data.get());
    }

    REF_getter<const bc_contract> getContractNoCreateConst(Cellable* root, IDatabase* db, const CONTRACT_id& name)
    {
        MUTEX_INSPECTOR;
        auto cc = getByPathNoCreate(root, db, getContractPath(name));
        if (!cc.valid())
            return NULL;
        if (!cc->data.valid())
            return NULL;
        return dynamic_cast<const bc_contract*>(cc->data.get());
    }

    REF_getter<bc_contract_data> getContractDataOrCreate(Cellable* root, IDatabase* db, const CONTRACT_id& con, const CONTRACT_DATA_id& name, Rollback* roll, int ttl, time_t create_time)
    {
        MUTEX_INSPECTOR;
        auto cc = getByPathOrCreate(root, db, getContractDataPath(con, name), roll);
        if (!cc.valid())
            throw CommonError("if(!cc.valid())");
        if (cc->data.valid())
            throw CommonError("if(cc->data.valid())");
        REF_getter<bc_contract_data> bc = new bc_contract_data(cc.get());
        bc->ttl = ttl;
        bc->update_time(create_time);
        cc->data = bc.get();
        cc->payload_ctor_idx = hsh::bc_contract_data;
        cc->data->setDirty(roll);
        return bc;
    }

    REF_getter<bc_contract_data> getContractDataNoCreate(Cellable* root, IDatabase* db, const CONTRACT_id& con, const CONTRACT_DATA_id& key)
    {
        MUTEX_INSPECTOR;
        auto cc = getByPathNoCreate(root, db, getContractDataPath(con, key));
        if (!cc.valid())
            return NULL;
        if (!cc->data.valid())
            return NULL;
        return dynamic_cast<bc_contract_data*>(cc->data.get());
    }

    REF_getter<const bc_contract_data> getContractDataNoCreateConst(Cellable* root, IDatabase* db, const CONTRACT_id& con, const CONTRACT_DATA_id& key)
    {
        MUTEX_INSPECTOR;
        auto cc = getByPathNoCreate(root, db, getContractDataPath(con, key));
        if (!cc.valid())
            return NULL;
        if (!cc->data.valid())
            return NULL;
        return dynamic_cast<const bc_contract_data*>(cc->data.get());
    }

    REF_getter<bc_contract_balance> getContractBalanceOrCreate(Cellable* root, IDatabase* db, const CONTRACT_id& contract, Rollback* roll)
    {
        MUTEX_INSPECTOR;
        auto cc = getByPathOrCreate(root, db, getContractBalancePath(contract), roll);
        if (!cc.valid())
            throw CommonError("if(!cc.valid())");
        if (cc->data.valid())
            return dynamic_cast<bc_contract_balance*>(cc->data.get());
        REF_getter<bc_contract_balance> bc = new bc_contract_balance(cc.get());
        cc->data = bc.get();
        cc->payload_ctor_idx = hsh::bc_contract_balance;
        cc->data->setDirty(roll);
        return bc;
    }

    REF_getter<bc_contract_balance> getContractBalanceNoCreate(Cellable* root, IDatabase* db, const CONTRACT_id& contract)
    {
        MUTEX_INSPECTOR;
        auto cc = getByPathNoCreate(root, db, getContractBalancePath(contract));
        if (!cc.valid())
            return NULL;
        if (!cc->data.valid())
            return NULL;
        return dynamic_cast<bc_contract_balance*>(cc->data.get());
    }

    REF_getter<const bc_contract_balance> getContractBalanceNoCreateConst(Cellable* root, IDatabase* db, const CONTRACT_id& contract)
    {
        MUTEX_INSPECTOR;
        auto v = getContractBalanceNoCreate(root, db, contract);
        if (!v.valid())
            return NULL;
        return dynamic_cast<const bc_contract_balance*>(v.get());
    }

// ============================================================
// node / node list
// ============================================================
    REF_getter<bc_node> getNodeNoCreate(Cellable* root, IDatabase* db, const NODE_id& name)
    {
        MUTEX_INSPECTOR;
        auto cc = getByPathNoCreate(root, db, getNodePath(name));
        if (!cc.valid())
            return NULL;
        if (!cc->data.valid())
            return NULL;
        return dynamic_cast<bc_node*>(cc->data.get());
    }

    REF_getter<const bc_node> getNodeNoCreateConst(Cellable* root, IDatabase* db, const NODE_id& name)
    {
        auto n = getNodeNoCreate(root, db, name);
        if (!n.valid())
            return NULL;
        return dynamic_cast<const bc_node*>(n.get());
    }

    REF_getter<bc_nodelist> getNodeListNoCreate(Cellable* root, IDatabase* db)
    {
        MUTEX_INSPECTOR;
        auto cc = getByPathNoCreate(root, db, getPath("NODE LIST"));
        if (!cc.valid())
            return NULL;
        if (!cc->data.valid())
            return NULL;
        return dynamic_cast<bc_nodelist*>(cc->data.get());
    }

    REF_getter<const bc_nodelist> getNodeListNoCreateConst(Cellable* root, IDatabase* db)
    {
        auto n = getNodeListNoCreate(root, db);
        if (!n.valid())
            return NULL;
        return dynamic_cast<const bc_nodelist*>(n.get());
    }

    REF_getter<bc_nodelist> getNodeListOrCreate(Cellable* root, IDatabase* db, Rollback* roll)
    {
        MUTEX_INSPECTOR;
        auto cc = getByPathOrCreate(root, db, getPath("NODE LIST"), roll);
        if (!cc.valid())
            return NULL;
        if (cc->data.valid())
        {
            return dynamic_cast<bc_nodelist*>(cc->data.get());
        }
        REF_getter<bc_nodelist> u = new bc_nodelist(cc.get());
        cc->data = u.get();
        cc->payload_ctor_idx = hsh::bc_nodelist;
        return u;
    }

    REF_getter<bc_node> addNode(Cellable* root, IDatabase* db, const NODE_id& name, Rollback* roll)
    {
        MUTEX_INSPECTOR;
        auto nl = getNodeListNoCreate(root, db);
        if (nl->count(name))
            throw CommonError("if(!nl->list.count(name))");
        nl->insert(name);
        nl->setDirty(roll);

        auto cc = getByPathOrCreate(root, db, getNodePath(name), roll);
        if (cc->data.valid())
            throw CommonError("if(cc->data.valid())");

        REF_getter<bc_node> n = new bc_node(cc.get());
        cc->data = n.get();
        cc->payload_ctor_idx = hsh::bc_node;

        {
            auto st = getStateNoCreate(root, db);
            if (st.valid())
            {
                
                st->node_count++;
            }
            st->setDirty(roll);
        }
        return n;
    }

    std::vector<REF_getter<bc_node>> getAllNodes(Cellable* root, IDatabase* db)
    {
        MUTEX_INSPECTOR;
        auto nl = getNodeListNoCreate(root, db);
        if (!nl.valid())
            throw CommonError("if(!nl.valid())");

        std::vector<REF_getter<bc_node>> vv;
        auto ll = nl->getList();
        for (auto& z : ll)
        {
            vv.push_back(getNodeNoCreate(root, db, z));
        }
        return vv;
    }

// ============================================================
// values / votes / state
// ============================================================
    REF_getter<bc_values> getValuesOrCreate(Cellable* root, IDatabase* db, Rollback* roll)
    {
        MUTEX_INSPECTOR;
        auto l = getByPathOrCreate(root, db, getPath("VALUES"), roll);
        if (!l.valid())
            throw CommonError("if(!l.valid())");
        if (l->data.valid())
        {
            auto* p = dynamic_cast<bc_values*>(l->data.get());
            return p;
        }
        REF_getter<bc_values> v = new bc_values(l.get());
        l->data = v.get();
        l->payload_ctor_idx = hsh::bc_values;
        return v;
    }

    REF_getter<bc_values> getValuesNoCreate(Cellable* root, IDatabase* db)
    {
        MUTEX_INSPECTOR;
        auto l = getByPathNoCreate(root, db, getPath("VALUES"));
        if (!l.valid())
            return NULL;
        if (!l->data.valid())
            return NULL;
        return dynamic_cast<bc_values*>(l->data.get());
    }

    REF_getter<const bc_values> getValuesNoCreateConst(Cellable* root, IDatabase* db)
    {
        MUTEX_INSPECTOR;
        auto v = getValuesNoCreate(root, db);
        if (!v.valid())
            return NULL;
        return dynamic_cast<const bc_values*>(v.get());
    }

    REF_getter<bc_votes> getVotesOrCreate(Cellable* root, IDatabase* db, Rollback* roll)
    {
        MUTEX_INSPECTOR;
        auto l = getByPathOrCreate(root, db, getPath("VOTES"), roll);
        if (!l.valid())
            throw CommonError("if(!l.valid())");
        if (l->data.valid())
        {
            return dynamic_cast<bc_votes*>(l->data.get());
        }
        REF_getter<bc_votes> v = new bc_votes(l.get());
        l->data = v.get();
        l->payload_ctor_idx = hsh::bc_votes;
        return v;
    }

    REF_getter<bc_votes> getVotesNoCreate(Cellable* root, IDatabase* db)
    {
        MUTEX_INSPECTOR;
        auto l = getByPathNoCreate(root, db, getPath("VOTES"));
        if (!l.valid())
            return NULL;
        if (!l->data.valid())
            return NULL;
        return dynamic_cast<bc_votes*>(l->data.get());
    }

    REF_getter<const bc_votes> getVotesNoCreateConst(Cellable* root, IDatabase* db)
    {
        MUTEX_INSPECTOR;
        auto v = getVotesNoCreate(root, db);
        if (!v.valid())
            return NULL;
        return dynamic_cast<const bc_votes*>(v.get());
    }

    REF_getter<bc_state> getStateOrCreate(Cellable* root, IDatabase* db, Rollback* roll)
    {
        MUTEX_INSPECTOR;
        auto l = getByPathOrCreate(root, db, getStatePath(), roll);
        if (!l.valid())
            throw CommonError("if(!l.valid())");
        if (l->data.valid())
            return dynamic_cast<bc_state*>(l->data.get());
        REF_getter<bc_state> v = new bc_state(l.get());
        l->data = v.get();
        l->payload_ctor_idx = hsh::bc_state;
        return v;
    }

    REF_getter<bc_state> getStateNoCreate(Cellable* root, IDatabase* db)
    {
        MUTEX_INSPECTOR;
        auto l = getByPathNoCreate(root, db, getStatePath());
        if (!l.valid())
            return NULL;
        if (!l->data.valid())
            return NULL;
        return dynamic_cast<bc_state*>(l->data.get());
    }

    REF_getter<const bc_state> getStateNoCreateConst(Cellable* root, IDatabase* db)
    {
        MUTEX_INSPECTOR;
        auto l = getByPathNoCreate(root, db, getStatePath());
        if (!l.valid())
            return NULL;
        if (!l->data.valid())
            return NULL;
        return dynamic_cast<const bc_state*>(l->data.get());
    }
    std::vector<std::string> getContractActivityPath(const CONTRACT_id& name)
    {
        return getPath("CONTRACT_ACTIVITY_" + name.container);
    }

    REF_getter<bc_contract_activity> getContractActivityOrCreate(Cellable* root, IDatabase* db, const CONTRACT_id& name, Rollback* roll)
    {
        auto cc = getByPathOrCreate(root, db, getContractActivityPath(name), roll);
        if (!cc.valid()) throw CommonError("if(!cc.valid())");
        if (cc->data.valid()) return dynamic_cast<bc_contract_activity*>(cc->data.get());
        REF_getter<bc_contract_activity> bc = new bc_contract_activity(cc.get());
        bc->name_ = name.container;
        cc->data = bc.get();
        cc->payload_ctor_idx = hsh::bc_contract_activity;
        cc->data->setDirty(roll);
        return bc;
    }

    REF_getter<bc_contract_activity> getContractActivityNoCreate(Cellable* root, IDatabase* db, const CONTRACT_id& name)
    {
        auto cc = getByPathNoCreate(root, db, getContractActivityPath(name));
        if (!cc.valid()) return NULL;
        if (!cc->data.valid()) return NULL;
        return dynamic_cast<bc_contract_activity*>(cc->data.get());
    }

    REF_getter<const bc_contract_activity> getContractActivityNoCreateConst(Cellable* root, IDatabase* db, const CONTRACT_id& name)
    {
        auto v = getContractActivityNoCreate(root, db, name);
        if (!v.valid()) return NULL;
        return dynamic_cast<const bc_contract_activity*>(v.get());
    }
} // namespace cells
