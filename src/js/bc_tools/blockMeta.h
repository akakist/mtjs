#pragma once
#include "REF.h"
#include "NODE_id.h"
#include "bc_node.h"
#include "tree.h"
struct BlockMetaFull: public Refcountable
{
    std::map<NODE_id, uint64_t> prev_stakes;
    // std::map<NODE_id, blst_cpp::PublicKey> prevpks;
    // blst_cpp::PublicKey bls_pk;
    uint64_t getPrevStake(const NODE_id& n)
    {
        auto it=prev_stakes.find(n);
        if(it==prev_stakes.end())
            throw CommonError("if(it==prev_stakes.end())");
        return it->second;
    }
    std::vector<NODE_id> all_nodes_enabled;
    TreeNode tree_all_nodes;
    TreeNode tree_committe;
    std::map<NODE_id, size_t> position_of_node;
    std::set<NODE_id> committe_members;
    BlockMetaFull(): Refcountable("BlockMetaFull"){}
    
};
