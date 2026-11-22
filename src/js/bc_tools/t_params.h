
#pragma once
#include "md/md_attachment_data.h"
#include "md/md_ValidateBlockREQ.h"
#include "IDatabase.h"
#include "blockMeta.h"
#include "ADDRESS_id.h"
#include "THASH_id.h"
#include <cstdint>
#include "FeesSnapshot.h"
#include "emit_pack.h"
struct b_params
{
    b_params(IDatabase* _db): att_data(new MsgData::attachment_data()),db(_db) {}
    IDatabase* db;
    REF_getter<MsgData::ValidateBlockREQ> validateBlockREQ;
    REF_getter<MsgData::attachment_data> att_data;
    int64_t node_rewards=0;
    REF_getter<BlockMetaFull> meta;
    FeesSnapshot fees;

    // эмиты — единый бинарный формат (см. emit_pack.h), без JSON-строк
    void emit_cmd(const THASH_id& txId, int seqId, const std::string& command, const std::string& payload)
    {
        att_data->blockRoot.children[base16::encode(txId.container)].children[std::to_string(seqId)].emits.push_back({command, payload});
    }
    void emit_tx_cmd(const THASH_id& txId, const std::string& command, const std::string& payload)
    {
        att_data->blockRoot.children[base16::encode(txId.container)].emits.push_back({command, payload});
    }
    void emit_block_cmd(const std::string& command, const std::string& payload)
    {
        att_data->blockRoot.emits.push_back({command, payload});
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
    b_params* b;
    std::set<REF_getter<data_base>> *deleted_granules;
    // FeesSnapshot fees;
    // для msg-геттеров (источник истины — t_params; jsiMsg не собираем)
    // int64_t  block_timestamp = 0;   // из prev_block->heart_beat->block_timestamp
    // int64_t  block_height__Z = 0;   // TBD
    int64_t  nonce           = 0;   // из TX
    int64_t  chain_id        = 0;   // из TX
    int64_t  tx_index        = 0;   // TBD

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
            for (auto it = z.first->children_ptrs.begin();
                 it != z.first->children_ptrs.end(); )
            {
                if (!z.first->children_hashes.count(it->first))
                {
                    keepalive.push_back(it->second);   // чтобы не оставить dangling
                    it = z.first->children_ptrs.erase(it);
                }
                else
                    ++it;
            }

            z.first->markDirty(z.first->last_block_timestamp);
        }
    }
};



