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
#include "md/md_ValidateBlockRSP.h"
#include "md/md_BlockDiffValidateRSP.h"
#include "md/md_BlockAccepted2REQ.h"
#include "bc_contract_balance.h"
#include "bc_state.h"

struct IDatabase: public Refcountable
{

    virtual std::string getDbName()=0;

    virtual int getGranule(const std::string& k, std::string* v)=0;
    virtual int write_granules_batch(const _db_to_save &v)=0;
    virtual int eraseGranule(const std::string& k) = 0;   // физическое удаление (только компакция)

    IDatabase():Refcountable("Idatabse") {}
};

REF_getter<Cellable>  getRoot(IDatabase *db, const REF_getter<MsgData::BlockAccepted2REQ>& pb);
REF_getter<MsgData::BlockAccepted2REQ> load_last_block(IDatabase *db);
REF_getter<MsgData::BlockInfo> load_signed_block(IDatabase *db);
void store_signed_block(IDatabase *db, const REF_getter<MsgData::BlockInfo> &bi);

/// Голоса узла за высоту (cli): blockInfo+sig и ba_hash+sig.
/// Пишется сразу при изменении, подгружается на старте; тело не хранится.
struct cli_vote_record: public Refcountable
{
    int64_t epoch = 0;
    THASH_id base_root;
    REF_getter<MsgData::ValidateBlockRSP> block_rsp;    // vote-only (body пустое)
    REF_getter<MsgData::BlockDiffValidateRSP> diff_rsp;
    cli_vote_record(): Refcountable("cli_vote_record") {}
    std::string pack() const;
    bool unpack(const std::string& buf);
};
REF_getter<cli_vote_record> load_cli_vote(IDatabase *db);
void store_cli_vote(IDatabase *db, const REF_getter<cli_vote_record> &rec);
void erase_cli_vote(IDatabase *db);
