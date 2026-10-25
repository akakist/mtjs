
#pragma once
#include "md/md_attachment_data.h"
#include "md/md_ValidateBlockREQ.h"
#include "IDatabase.h"
#include "blockMeta.h"
#include "ADDRESS_id.h"
#include "THASH_id.h"
#include <cstdint>

struct b_params
{
    b_params(IDatabase* _db): att_data(new MsgData::attachment_data()),db(_db) {}
    IDatabase* db;
    REF_getter<MsgData::ValidateBlockREQ> validateBlockREQ;
    REF_getter<MsgData::attachment_data> att_data;
    uint64_t node_rewards=0;
    REF_getter<BlockMetaFull> meta;

    void emit_command(const THASH_id& txId, int seqId, const std::string& command, const char* fmt, ...)
    {
        va_list ap; char str[1024]; va_start(ap, fmt);
        int len = vsnprintf(str, sizeof(str), fmt, ap);
        bool overflow = (len >= (int)sizeof(str)); va_end(ap);
        // logErr2("emit_command : %с %s", command.c_str(),str);
        att_data->blockRoot.children[base16::encode(txId.container)].children[std::to_string(seqId)].emits.push_back({command, overflow?"\"overflow\"":str});
    }
    void emit_tx(const THASH_id& txId, const std::string& command, const char* fmt, ...)
    {
        va_list ap; char str[1024]; va_start(ap, fmt);
        int len = vsnprintf(str, sizeof(str), fmt, ap);
        bool overflow = (len >= (int)sizeof(str)); va_end(ap);
        att_data->blockRoot.children[base16::encode(txId.container)].emits.push_back({command, overflow?"\"overflow\"":str});
    }
    void emit_block(const std::string& command, const char* fmt, ...)
    {
        va_list ap; char str[1024]; va_start(ap, fmt);
        int len = vsnprintf(str, sizeof(str), fmt, ap);
        bool overflow = (len >= (int)sizeof(str)); va_end(ap);
        att_data->blockRoot.emits.push_back({command, overflow?"\"overflow\"":str});
    }
};

struct t_params
{
    t_params(const REF_getter<IDatabase>& d) : db(d) {}
    REF_getter<IDatabase> db;
    ADDRESS_id senderAddress;       // sender (hex берём через base16::encode(addr))
    REF_getter<MsgData::TX> tx;
    uint64_t epoch = 0;
    THASH_id tx_id;
    uint64_t gasUsed = 0;
    uint64_t gasLimit = 0;
    uint64_t value = 0;
    Rollback *roll = NULL;
    Dirty dirty;

    // для msg-геттеров (источник истины — t_params; jsiMsg не собираем)
    int64_t  block_timestamp = 0;   // из prev_block->heart_beat->block_timestamp
    int64_t  block_height    = 0;   // TBD
    int64_t  nonce           = 0;   // из TX
    int64_t  chain_id        = 0;   // из TX
    int64_t  tx_index        = 0;   // TBD

    void markDirty(data_base *p) { p->setDirty(roll); dirty.add(p); }
    void rollback()
    {
        for(auto &z: roll->data) {
            inBuffer in(z.second);
            { M_LOCK(z.first->mx); z.first->unpack_mx(in); }
        }
    }
};



