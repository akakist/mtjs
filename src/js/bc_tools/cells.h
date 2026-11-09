/// cells.h

#pragma once
#include <string>
#include <vector>
#include <deque>
#include <set>
#include "REF.h"
#include "cellable.h"
#include "THASH_id.h"
#include "CONTRACT_id.h"
#include "CONTRACT_DATA_id.h"
#include "NODE_id.h"
#include "ADDRESS_id.h"
#include "Rollback.h"
#include "db_to_save.h"
#include "bc_address_state.h"
#include "bc_contract.h"
#include "bc_granule.h"
#include "bc_contract_balance.h"
#include "bc_node.h"
#include "bc_nodelist.h"
#include "bc_values.h"
#include "bc_vote.h"
#include "bc_state.h"
#include "bc_activity.h"

struct IDatabase;

namespace cells {

// ============================================================
// path computation (pure, no db/root)
// ============================================================
    std::vector<std::string> getPath(const std::string& name);
    std::vector<std::string> getPathFromHash(const THASH_id& h);

    std::string getAddressStateKey(const ADDRESS_id& addr);
    std::string getContractKey(const CONTRACT_id& name);
    std::string getContractDataKey(const CONTRACT_id& contract, const CONTRACT_DATA_id& dataId);
    std::string getContractBalanceKey(const CONTRACT_id& contract);

    std::string getNodeKey(const NODE_id& name);

    std::string getStateKey();



    std::vector<std::string> getAddressStatePath(const ADDRESS_id& addr);
    std::vector<std::string> getContractPath(const CONTRACT_id& name);
    std::vector<std::string> getContractDataPath(const CONTRACT_id& contract, const CONTRACT_DATA_id& dataId);
    std::string getContractDataKey(const CONTRACT_id& contract, const CONTRACT_DATA_id& dataId);

    std::vector<std::string> getNodePath(const NODE_id& name);
    std::vector<std::string> getContractBalancePath(const CONTRACT_id& contract);
    std::vector<std::string> getStatePath();

// ============================================================
// path / cell traversal
// ============================================================
    REF_getter<Cellable> getByPathOrCreate(Cellable* root, IDatabase* db, const std::vector<std::string>& v, Rollback* roll);
    REF_getter<Cellable> getByPathOrCreate(Cellable* root, IDatabase* db, const std::deque<std::string>& v, Rollback* roll);
    REF_getter<Cellable> getByPathNoCreate(Cellable* root, IDatabase* db, const std::vector<std::string>& v);

    REF_getter<Cellable> getByDbIdNoCreate(Cellable* root, IDatabase* db, const THASH_id& h);

    REF_getter<Cellable> replaceLeafOrCreate(Cellable* cur, IDatabase* db, const std::string& id, Rollback* roll);
    REF_getter<Cellable> replaceLeafNoCreate(Cellable* cur, IDatabase* db, const std::string& id);
    REF_getter<data_base> replaceLeaf(Cellable* root, IDatabase* db, const THASH_id& name, Rollback* roll, const std::string& buffer);

// ============================================================
// address state
// ============================================================
    REF_getter<bc_address_state> getAddressStateOrCreate(Cellable* root, IDatabase* db, const ADDRESS_id& addr, Rollback* roll);
    REF_getter<bc_address_state> getAddressStateNoCreate(Cellable* root, IDatabase* db, const ADDRESS_id& addr);
    REF_getter<const bc_address_state> getAddressStateNoCreateConst(Cellable* root, IDatabase* db, const ADDRESS_id& addr);

// ============================================================
// contract / contract data / contract balance
// ============================================================
    REF_getter<bc_contract> getContractOrCreate(Cellable* root, IDatabase* db, const CONTRACT_id& name, Rollback* roll);
    REF_getter<bc_contract> getContractNoCreate(Cellable* root, IDatabase* db, const CONTRACT_id& name);
    REF_getter<const bc_contract> getContractNoCreateConst(Cellable* root, IDatabase* db, const CONTRACT_id& name);

    REF_getter<bc_granule> getContractDataOrCreate(Cellable* root, IDatabase* db, const CONTRACT_id& con, const CONTRACT_DATA_id& key, Rollback* roll, int ttl, time_t create_time);
    REF_getter<bc_granule> getContractDataNoCreate(Cellable* root, IDatabase* db, const CONTRACT_id& con, const CONTRACT_DATA_id& key);
    REF_getter<const bc_granule> getContractDataNoCreateConst(Cellable* root, IDatabase* db, const CONTRACT_id& con, const CONTRACT_DATA_id& key);

    REF_getter<bc_contract_balance> getContractBalanceOrCreate(Cellable* root, IDatabase* db, const CONTRACT_id& contract, Rollback* roll);
    REF_getter<bc_contract_balance> getContractBalanceNoCreate(Cellable* root, IDatabase* db, const CONTRACT_id& contract);
    REF_getter<const bc_contract_balance> getContractBalanceNoCreateConst(Cellable* root, IDatabase* db, const CONTRACT_id& contract);

// ============================================================
// node / node list
// ============================================================
    REF_getter<bc_node> getNodeNoCreate(Cellable* root, IDatabase* db, const NODE_id& name);
    REF_getter<const bc_node> getNodeNoCreateConst(Cellable* root, IDatabase* db, const NODE_id& name);
    REF_getter<bc_node> addNode(Cellable* root, IDatabase* db, const NODE_id& name, Rollback* roll);

    REF_getter<bc_nodelist> getNodeListOrCreate(Cellable* root, IDatabase* db, Rollback* roll);
    REF_getter<bc_nodelist> getNodeListNoCreate(Cellable* root, IDatabase* db);
    REF_getter<const bc_nodelist> getNodeListNoCreateConst(Cellable* root, IDatabase* db);

    std::vector<REF_getter<bc_node>> getAllNodes(Cellable* root, IDatabase* db);

// ============================================================
// values / votes / state
// ============================================================
    REF_getter<bc_values> getValuesOrCreate(Cellable* root, IDatabase* db, Rollback* roll);
    REF_getter<bc_values> getValuesNoCreate(Cellable* root, IDatabase* db);
    REF_getter<const bc_values> getValuesNoCreateConst(Cellable* root, IDatabase* db);

    REF_getter<bc_votes> getVotesOrCreate(Cellable* root, IDatabase* db, Rollback* roll);
    REF_getter<bc_votes> getVotesNoCreate(Cellable* root, IDatabase* db);
    REF_getter<const bc_votes> getVotesNoCreateConst(Cellable* root, IDatabase* db);

    REF_getter<bc_state> getStateOrCreate(Cellable* root, IDatabase* db, Rollback* roll);
    REF_getter<bc_state> getStateNoCreate(Cellable* root, IDatabase* db);
    REF_getter<const bc_state> getStateNoCreateConst(Cellable* root, IDatabase* db);


    // activity
    std::vector<std::string> getActivityPath(const THASH_id& granule_dbid);

    REF_getter<bc_activity> getActivityNoCreate(Cellable* root, IDatabase* db, const std::string& granule_dbid);

    REF_getter<bc_activity> getActivityOrCreate(Cellable* root, IDatabase* db,
                                        const std::string& granule_dbid, const std::set<std::string> &additional,
                                        int ttl, time_t now, Rollback* roll);

    void touchActivity(Cellable* root, IDatabase* db,
                    const std::string& granule_dbid,
                    Rollback* roll, time_t now);
} // namespace cells
