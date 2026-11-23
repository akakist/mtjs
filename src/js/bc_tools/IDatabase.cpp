#include "cellable.h"
#include "IDatabase.h"
#include "md_BlockAccepted2REQ.h"
#include "md_ValidateBlockRSP.h"
#include "md_BlockDiffValidateRSP.h"
#include "normalizeNick.h"


REF_getter<MsgData::BlockAccepted2REQ> load_last_block(IDatabase *db)
{
    std::string buf;
    auto r=db->getGranule("...last_block...",&buf);
    REF_getter<MsgData::BlockAccepted2REQ> last_block=nullptr;
    if(buf.size())
    {
        last_block=new MsgData::BlockAccepted2REQ;
        inBuffer in(buf);
        // 
        last_block->unpack2(in);

    }
    return last_block;
}
REF_getter<MsgData::BlockInfo> load_signed_block(IDatabase *db)
{
    std::string buf;
    db->getGranule("...signed_block...",&buf);
    REF_getter<MsgData::BlockInfo> bi=nullptr;
    if (buf.size()) {
        bi=new MsgData::BlockInfo;
        inBuffer in(buf);
        bi->unpack(in);
    }
    return bi;
}
void store_signed_block(IDatabase *db, const REF_getter<MsgData::BlockInfo> &bi)
{
    // Не пишем синхронно из обработчика: запись идёт через отложенный батч узла
    // (как last_block). См. client_node.cpp: db_to_save_Z.add("...signed_block...", ...).
    (void)db; (void)bi;
}
REF_getter<Cellable> getRoot(IDatabase *db, const REF_getter<MsgData::BlockAccepted2REQ>& pb)
{
    REF_getter<Cellable> r = new Cellable(NULL,"");
    std::string lb;
    int err = db->getGranule("", &lb);
    if (!err && lb.size()) {
        if (pb.valid()) {
            auto h = blake2b_hash(lb);
            if (pb->blockAcceptedREQ->blockInfo->new_root_hash1 == h) {
                inBuffer in(lb); r->unpack_mx(in);
            } else {
                logErr2("block hash not matched %s %s",
                    base16::encode(pb->blockAcceptedREQ->blockInfo->new_root_hash1.container).c_str(),
                    base16::encode(h.container).c_str());
                return new Cellable(NULL,"");
            }
        } else {
            // genesis / first run / sync: root из RocksDB без сверки
            inBuffer in(lb); r->unpack_mx(in);
        }
    }
    return r;
}
// ── cli: голоса узла за высоту ───────────────────────────────────────────
static const char* CLI_VOTE_KEY = "...cli_vote...";

std::string cli_vote_record::pack() const
{
    outBuffer b;
    b << (int)1;                 // версия формата
    b << epoch;
    b << base_root;
    b << (int)(block_rsp.valid() ? 1 : 0);
    if (block_rsp.valid())
    {
        b << block_rsp->blockInfo;
        b << block_rsp->sig;
        b << block_rsp->node_validator;
    }
    b << (int)(diff_rsp.valid() ? 1 : 0);
    if (diff_rsp.valid())
    {
        b << diff_rsp->blockAcceptedHash;
        b << diff_rsp->sig34;
        b << diff_rsp->node_validator;
    }
    return b.asString()->container;
}

bool cli_vote_record::unpack(const std::string& buf)
{
    inBuffer in(buf);
    int ver = 0;
    in >> ver;
    if (ver != 1) return false;
    in >> epoch;
    in >> base_root;
    int has_block = 0;
    in >> has_block;
    if (has_block)
    {
        block_rsp = new MsgData::ValidateBlockRSP();
        in >> block_rsp->blockInfo;
        in >> block_rsp->sig;
        in >> block_rsp->node_validator;
        block_rsp->att_data = new MsgData::attachment_data();   // тело не храним
    }
    int has_diff = 0;
    in >> has_diff;
    if (has_diff)
    {
        diff_rsp = new MsgData::BlockDiffValidateRSP();
        in >> diff_rsp->blockAcceptedHash;
        in >> diff_rsp->sig34;
        in >> diff_rsp->node_validator;
    }
    return true;
}

REF_getter<cli_vote_record> load_cli_vote(IDatabase *db)
{
    std::string buf;
    db->getGranule(CLI_VOTE_KEY, &buf);
    if (buf.empty()) return nullptr;
    auto rec = new cli_vote_record;
    if (!rec->unpack(buf)) return nullptr;
    return rec;
}

void store_cli_vote(IDatabase *db, const REF_getter<cli_vote_record> &rec)
{
    _db_to_save b;
    b.add(CLI_VOTE_KEY, rec->pack());
    db->write_granules_batch(b);
}

void erase_cli_vote(IDatabase *db)
{
    db->eraseGranule(CLI_VOTE_KEY);
}
