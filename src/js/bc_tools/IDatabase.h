#pragma once
#include <string>
#include "REF.h"
#include "db_to_save.h"
#include "THASH_id.h"
#include "THASH_id.h"
#include "bc_contract.h"
#include "bc_address_state.h"
#include "bc_granule.h"
#include "bc_node.h"
#include "bc_nodelist.h"
#include "bc_values.h"
#include "bc_vote.h"
#include "CONTRACT_id.h"
#include "CONTRACT_DATA_id.h"
#include "md/md_BlockAcceptedREQ.h"
#include "md/md_BlockAccepted2REQ.h"
#include "bc_contract_balance.h"
#include "bc_state.h"

struct IDatabase: public Refcountable
{

    virtual std::string getDbName()=0;

    virtual int getGranule(const std::string& k, std::string* v)=0;
    virtual int write_granules_batch(const _db_to_save &v)=0;
    virtual int deleteGranule(const std::string& k) = 0;

    IDatabase():Refcountable("Idatabse") {}
};

REF_getter<Cellable>  getRoot(IDatabase *db, const REF_getter<MsgData::BlockAccepted2REQ>& pb);
REF_getter<MsgData::BlockAccepted2REQ> load_last_block(IDatabase *db);
