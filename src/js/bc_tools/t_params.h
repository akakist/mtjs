
#pragma once
#include "md/md_attachment_data.h"
#include "md/md_ValidateBlockREQ.h"
#include "IDatabase.h"
#include "blockMeta.h"
#include "ADDRESS_id.h"
#include "THASH_id.h"
#include <cstdint>
#include "FeesSnapshot.h"
struct b_params
{
    b_params(IDatabase* _db): att_data(new MsgData::attachment_data()),db(_db) {}
    IDatabase* db;
    REF_getter<MsgData::ValidateBlockREQ> validateBlockREQ;
    REF_getter<MsgData::attachment_data> att_data;
    int64_t node_rewards=0;
    REF_getter<BlockMetaFull> meta;
    FeesSnapshot fees;

    void emit_command(const THASH_id& txId, int seqId, const std::string& command, const char* fmt, ...)
    {
        va_list ap; char str[1024]; va_start(ap, fmt);
        int len = vsnprintf(str, sizeof(str), fmt, ap);
        bool overflow = (len >= (int)sizeof(str)); va_end(ap);
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
    int64_t epoch = 0;
    THASH_id tx_id;
    int64_t gasUsed = 0;
    int64_t gasLimitGas = 0;
    int64_t value = 0;
    Rollback *roll = NULL;
    Dirty dirty_1;
    b_params* b;
    std::set<REF_getter<data_base>> *deleted_granules;
    // FeesSnapshot fees;
    // для msg-геттеров (источник истины — t_params; jsiMsg не собираем)
    // int64_t  block_timestamp = 0;   // из prev_block->heart_beat->block_timestamp
    // int64_t  block_height__Z = 0;   // TBD
    int64_t  nonce           = 0;   // из TX
    int64_t  chain_id        = 0;   // из TX
    int64_t  tx_index        = 0;   // TBD

    void markDirty(data_base *p) { p->setDirty(roll, b->validateBlockREQ->block_timestamp); dirty_1.add(p); }
    // Держим ссылки на ячейки, убранные из дерева при откате, чтобы
    // не оставить висячих указателей у тех, кто ещё держит data_base*.
    std::vector<REF_getter<Cellable>> keepalive;

    void rollback()
    {
        for(auto &z: roll->data) {
            inBuffer in(z.second);
            {  z.first->unpack_mx(in); }
            if (getenv("MTJS_TREE_DUMP"))
                logErr2("[TRACE rollback] id=%s ts=%lld h=%s",
                        base16::encode(z.first->getDbId()).substr(0,16).c_str(),
                        (long long)z.first->last_block_timestamp.v,
                        base16::encode(blake2b_hash(z.first->getBuffer_mx()).container).substr(0,16).c_str());

            // unpack_mx восстановил children_hashes_mx. Указатели на детей,
            // которых в восстановленных хешах нет (созданы в упавшей tx),
            // убираем — иначе calc_tree_hash вернёт их обратно в хеши.
            // Остальные указатели не трогаем: сохранённое состояние остаётся рабочим.
            for (auto it = z.first->children_ptrs_mx.begin();
                 it != z.first->children_ptrs_mx.end(); )
            {
                if (!z.first->children_hashes_mx.count(it->first))
                {
                    keepalive.push_back(it->second);   // чтобы не оставить dangling
                    it = z.first->children_ptrs_mx.erase(it);
                }
                else
                    ++it;
            }

            z.first->setDirty__(nullptr, z.first->last_block_timestamp);
        }
    }
};



