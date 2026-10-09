#pragma once
#include "REF.h"
#include "NODE_id.h"
#include "bc_node.h"
#include "tree.h"
struct BlockMetaFull: public Refcountable
{
    // std::map<NODE_id,REF_getter<bc_node>> all_nodes;
    // std::set<NODE_id> full_broadcast;
    // std::map<NODE_id, uint64_t> node_stakes;
    // uint64_t all_nodes_full_stake=0;
    // uint64_t committe_full_stake=0;
    TreeNode tree_all_nodes;
    TreeNode tree_committe;
    std::map<NODE_id, size_t> position_of_node;
    std::set<NODE_id> committe_members;
    // REF_getter<bc_node> getNode(const NODE_id &n)
    // {
    //     auto it=all_nodes.find(n);
    //     if(it==all_nodes.end())
    //         throw CommonError("if(n==nodes.end())");
    //     return it->second;
    // }
    // uint64_t getStake(const NODE_id &n)        
    // {
    //     auto it=node_stakes.find(n);
    //     if(it==node_stakes.end())
    //         throw CommonError("if(n==nodes.end())");
    //     return it->second;
    // }
    BlockMetaFull(): Refcountable("BlockMetaFull"){}
    
};
