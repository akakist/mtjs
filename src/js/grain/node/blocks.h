#pragma once

#include "NODE_id.h"
#include "REF.h"
#include "blockMeta.h"
#include "md/md_BlockStatusREQ.h"

namespace Node
{
    class Service;
}

/// Проверка подписей одного субблока: агрегаты комитета и не-комитета.
///
/// Состав групп, ключи и стейки берутся из меты BlockMetaFull:
///   * комитет    — mf->committe_members;
///   * не-комитет — mf->noncommitte_members (enabled-ноды вне комитета);
///   * BLS-ключи  — mf->bls_pks (через getBlsPk);
///   * стейки     — mf->prev_stakes; знаменатели — full_stake_committe/full_stake_all.
///
/// Проверяется:
///   * ключ записи == hash(BlockInfo), а offer_hash/heart_beat согласованы с субблоком;
///   * каждый подписант входит в свою группу и не голосовал дважды
///     (в том числе как комитет и как не-комитет одновременно);
///   * агрегированная BLS-подпись сходится с ключами перечисленных подписантов;
///   * кворум > QUORUM% по стейку — флаги has_committe_quorum/has_noncommite_quorum.
///
/// corrupt=true — субблок использовать нельзя: подписи или состав групп битые.
SB_result verify_subblock_sigs(const REF_getter<MsgData::SubBlock>& b,
                               const REF_getter<MsgData::HeartBeatREQ>& hb,
                               const REF_getter<BlockMetaFull>& mf,
                               Node::Service* ns);
