#pragma once
#include "REF.h"
#include "NODE_id.h"
#include "bc_node.h"
#include "tree.h"
struct BlockMetaFull: public Refcountable
{
    std::map<NODE_id, uint64_t> prev_stakes;
    uint64_t getPrevStake(const NODE_id& n) const
    {
        auto it=prev_stakes.find(n);
        if(it==prev_stakes.end())
            throw CommonError("if(it==prev_stakes.end())");
        return it->second;
    }
    uint64_t fullStake() const
    {
        uint64_t r=0;
        for(auto& z: prev_stakes)
        {
            r+=z.second;
        }
        return r;
    }
    std::vector<NODE_id> all_nodes_enabled;
    TreeNode tree_all_nodes;
    TreeNode tree_committe;
    std::map<NODE_id, size_t> position_of_node;
    std::set<NODE_id> committe_members;
    BlockMetaFull(): Refcountable("BlockMetaFull") {}

};
