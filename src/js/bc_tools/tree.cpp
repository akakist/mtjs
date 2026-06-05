#include <bits/stdc++.h>
#include "tree.h"
#include "NODE_id.h"
using namespace BroadcasterTree;



// Построение дерева: root + map узлов

void dump(const BroadcasterTree::TreeNode &t, int level, std::vector<std::pair<int,std::string>> & out)
{

    for(auto &c:t.children)
    {
        out.push_back({level,c.node.name.container+" stake="+c.node.stake_A.toString()});
        dump(c,level+1,out);
    }
}
BroadcasterTree::TreeNode BroadcasterTree::buildTree(const std::map<NODE_id,BroadcasterTree::Node>& nodes, const NODE_id& rootName_)
{
    if(nodes.empty())
        throw CommonError("if(nodes.empty())");

    std::deque<BroadcasterTree::TreeNode> ranked;
    for (auto& kv : nodes) {
        BroadcasterTree::TreeNode n;
        n.node=kv.second;
        ranked.emplace_back(kv.second);
    }
    sort(ranked.begin(), ranked.end(),
    [](const auto& a, const auto& b) {
        if(a.node.missed_rounds!=b.node.missed_rounds)
            return a.node.missed_rounds < b.node.missed_rounds; // меньше пропущенных раундов выше
        return a.node.stake_A > b.node.stake_A;
    });

    int idx_r=0;

    std::queue<BroadcasterTree::TreeNode*> q;

    BroadcasterTree::TreeNode fake;
    fake.node.name.container="fake";
    BroadcasterTree::TreeNode root=fake;
    q.push(&root);

    while (!q.empty() && idx_r<ranked.size()) {
        BroadcasterTree::TreeNode* cur = q.front();
        q.pop();



        for (int i = 0; i < 2; i++) {

            if(idx_r>=ranked.size())
                break;

            auto r=ranked[idx_r++];
            cur->children.emplace_back(r);   // копия в дерево
            BroadcasterTree::TreeNode* childPtr = &cur->children.back(); // стабильный адрес
            // nodes.erase(it);                          // удаляем из map
            q.push(childPtr);                         // добавляем в очередь
        }
    }
    return root;
}


#ifdef __1
int main() {
    std::map<std::string,BroadcasterTree::Node> nodes = {
        {"10.0.0.1:8000",{"10.0.0.1:8000",1}}, // Москва (root)
        {"192.168.1.10:9000",{"192.168.1.10:9000",2}}, // Лондон
        {"203.0.113.5:8080",{"203.0.113.5:8080",3}},    // Париж
        {"2001:db8::1:443",{"2001:db8::1:443",4}},    // Нью-Йорк (IPv6)
        {"172.16.0.2:7000",{"172.16.0.2:7000",6}},    // Токио
        {"198.51.100.7:6000",{"198.51.100.7:6000",7}}, // Лос-Анджелес
        {"2001:db8::2:8443",{"2001:db8::2:8443",8}},    // Амстердам (IPv6)
        {"203.0.113.9:5000",{"203.0.113.9:5000",9}},   // Рим
        {"192.0.2.11:3000",{"192.0.2.11:3000",11}},    // Пекин
        {"2001:db8::3:8081",{"2001:db8::3:8081",12}},  // Мехико (IPv6)
        {"198.51.100.12:5050",{"198.51.100.12:5050",13}}, // Сан-Франциско
        {"203.0.113.22:8082",{"203.0.113.22:8082",14}}, // Берлин
        {"2001:db8::4:9443",{"2001:db8::4:9443",15}},   // Хельсинки (IPv6)
        {"198.51.100.33:9090",{"198.51.100.33:9090",16}} // Копенгаген
    };

    const std::string rootIp = "10.0.0.1:8000";

    BroadcasterTree::TreeNode root = BroadcasterTree::buildTree(nodes, rootIp);
    nlohmann::json j = root.to_json();
    std::cout << j.dump(2) << std::endl;

    return 0;
}
#endif