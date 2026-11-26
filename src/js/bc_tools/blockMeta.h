#pragma once
#include "REF.h"
#include "NODE_id.h"
#include "bc_node.h"
#include "tree.h"
#include "time_types.h"

/// Половина слота: лидер рассылает новые субблоки только до этой границы
/// (ROUND_DURATION = 5с в md_BlockStatusREQ.h → 2.5с).
static constexpr int64_t SUBBLOCK_WINDOW_MKS = 2500000;
/// Если tx нет, раунд завершаем раньше — отдельный короткий дедлайн (1с).
static constexpr int64_t EMPTY_ROUND_WINDOW_MKS = 1000000;

struct BlockMetaFull: public Refcountable
{
    std::map<NODE_id, i128> prev_stakes;
    i128 getPrevStake(const NODE_id& n) const
    {
        auto it=prev_stakes.find(n);
        if(it==prev_stakes.end())
            throw CommonError("if(it==prev_stakes.end())");
        return it->second;
    }
    i128 fullStakeAll() const
    {
        i128 r=(i128)0;
        for(auto& z: prev_stakes)
        {
            r+=z.second;
        }
        return r;
    }
    i128 fullStakeCommitte() const
    {
        i128 r=(i128)0;
        for(auto& z: committe_members)
        {
            auto it=prev_stakes.find(z);
            if(it==prev_stakes.end())
                throw CommonError("if(it==prev_stakes.end()) @344");
            r+=it->second;
        }
        return r;
    }
    std::vector<NODE_id> all_nodes_enabled;
    std::vector<NodeElement> vne;
    // Валидаторы (комитет) в детерминированном порядке — из них выбирается лидер.
    // Лидер вне комитета не соберёт кворум и даст мёртвый слот.
    std::vector<NodeElement> committee_vne;

    TreeNode tree_all_nodes;
    TreeNode tree_committe;
    std::map<NODE_id, size_t> position_of_node;
    /// состав групп для проверки подписей субблоков
    std::set<NODE_id> committe_members;
    /// enabled-ноды вне комитета: подписывают результат применения диффов
    std::set<NODE_id> noncommitte_members;
    i128 full_stake_all;
    i128 full_stake_committe;
    std::map<NODE_id, blst_cpp::PublicKey> bls_pks;
    std::optional<blst_cpp::PublicKey> getBlsPk(const NODE_id& n) const
    {
        auto it=bls_pks.find(n);
        if(it!=bls_pks.end())
            return it->second;
        return std::nullopt;
    }
    NodeElement& leader()
    {
        return committee_vne[0];
    };
    std::string leader_ip;
    int round;
    /// Начало текущего раунда: prev_block_ts + round*ROUND_DURATION (заполняется в getMetaFull).
    TIME_mks round_start;

    /// Граница, до которой лидер рассылает новые субблоки (половина слота = 2.5с).
    TIME_mks subblock_deadline() const
    {
        return TIME_mks{round_start.v + SUBBLOCK_WINDOW_MKS};
    }
    /// Дедлайн пустого раунда (нет tx): завершаем раньше, чем половина слота.
    TIME_mks empty_round_deadline() const
    {
        return TIME_mks{round_start.v + EMPTY_ROUND_WINDOW_MKS};
    }
    /// true — ещё можно рассылать субблоки.
    bool can_broadcast_subblock(TIME_mks now) const
    {
        return now < subblock_deadline();
    }

    BlockMetaFull(): Refcountable("BlockMetaFull") {}

    bool isMemberCommitte(const NODE_id&n) const
    {
        return committe_members.count(n)!=0;
    }
    bool isMemberNonCommitte(const NODE_id&n) const
    {
        return noncommitte_members.count(n)!=0;
    }

};
