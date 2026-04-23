#pragma once
#include <string>
// #include <nlohmann/json.hpp>
#include <list>
#include "blake2bHasher.h"
#include "bigint.h"
#include "NODE_id.h"
// #include "auto_mpz_t.h"
namespace BroadcasterTree
{
    struct Node {
        NODE_id name;
        BigInt stake;
        std::string ip;
        void hash(Blake2bHasher&h)
        {
            h.update(name.container);
            h.update(stake.toString());
            h.update(ip);
        }
    };

    class TreeNode {
    public:
        TreeNode(const Node& b): node(b) {}
        Node node;
        std::list<TreeNode> children; // стабильные адреса элементов

        TreeNode()  {}

        void hash(Blake2bHasher&h)
        {
            node.hash(h);
            for(auto& z: children)
            {
                z.hash(h);
            }
        }
    };
    TreeNode buildTree(const std::map<NODE_id,Node>& nodes, const NODE_id& rootName);

} // namespace BroadcasterTree
inline outBuffer & operator<< (outBuffer& o,const BroadcasterTree::Node& t)
{
    o<<t.name<<t.stake<<t.ip;
    return o;
}
inline inBuffer & operator>> (inBuffer& o,BroadcasterTree::Node& t)
{
    o>>t.name>>t.stake>>t.ip;
    return o;
}

inline outBuffer & operator<< (outBuffer& o,const BroadcasterTree::TreeNode& t)
{
    o<<t.node<<t.children;
    return o;
}
inline inBuffer & operator>> (inBuffer& o,BroadcasterTree::TreeNode& t)
{
    o>>t.node>>t.children;
    return o;
}
void dump(const BroadcasterTree::TreeNode &t, int level, std::vector<std::pair<int,std::string>> & out);
