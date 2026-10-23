#pragma once
#include <quickjs.h>
#include <vector>
#include "REF.h"
#include "THASH_id.h"

namespace MsgData {
    struct BlockAccepted2REQ;
    struct attachment_data;
}

void register_block_class(JSContext* ctx);

JSValue js_block_new(JSContext* ctx,
                     const REF_getter<MsgData::BlockAccepted2REQ>& ba2,
                     const REF_getter<MsgData::attachment_data>& att,
                     const std::vector<THASH_id>& tx_hashes,
                     bool finalized);
