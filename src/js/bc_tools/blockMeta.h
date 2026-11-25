#pragma once
#include "REF.h"
#include "NODE_id.h"
#include "bc_node.h"
#include "tree.h"
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
    std::set<NODE_id> committe_members;
    i128 full_stake_all;
    i128 full_stake_committe;
    std::map<NODE_id, blst_cpp::PublicKey> bls_pks;
    std::optional<blst_cpp::PublicKey> getBlsPk(const NODE_id& n)
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
    BlockMetaFull(): Refcountable("BlockMetaFull") {}

};
