#include "cellable.h"
#include "IDatabase.h"
#include "md_BlockAccepted2REQ.h"
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