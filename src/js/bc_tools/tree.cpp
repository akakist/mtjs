#include <bits/stdc++.h>
#include <nlohmann/json.hpp>
#include "tree.h"
#include "NODE_id.h"
using namespace BroadcasterTree;
// using namespace std;
// using json = nlohmann::json;


// Хаверсин для расстояния (км)
// double haversine(double lat1, double lon1, double lat2, double lon2) {
//     const double R = 6371.0;
//     auto toRad = [](double d){ return d * M_PI / 180.0; };
//     double dLat = toRad(lat2 - lat1);
//     double dLon = toRad(lon2 - lon1);
//     double a = sin(dLat/2)*sin(dLat/2) +
//                cos(toRad(lat1))*cos(toRad(lat2)) *
//                sin(dLon/2)*sin(dLon/2);
//     double c = 2 * asin(sqrt(a));
//     return R * c;
// }

// Построение дерева: root + map узлов

void dump(const BroadcasterTree::TreeNode &t, int level, std::vector<std::pair<int,std::string>> & out)
{

    for(auto &c:t.children)
    {
        out.push_back({level,c.node.name.container+" stake="+c.node.stake.toString()});
        dump(c,level+1,out);
    }
}
BroadcasterTree::TreeNode BroadcasterTree::buildTree(const std::map<NODE_id,BroadcasterTree::Node>& nodes, const NODE_id& rootName_)
{
    // logErr2("root name %s",rootName.c_str());
    // Проверка наличия корня
    // auto itRoot = nodes.find(rootName);
    // if (itRoot == nodes.end()) {
    //     throw std::runtime_error("Root name not found in nodes");
    // }
    // BroadcasterTree::TreeNode root;
    // root.node.name="FAKE";
    // BroadcasterTree::TreeNode root(itRoot->second);
    // nodes.erase(itRoot->first);
    if(nodes.empty())
        throw CommonError("if(nodes.empty())");

    std::deque<BroadcasterTree::TreeNode> ranked;
    // ranked.reserve(nodes.size());
    for (auto& kv : nodes) {
        // if(kv.second.name == rootName)
        //     continue;
        BroadcasterTree::TreeNode n;
        n.node=kv.second;
        ranked.emplace_back(kv.second);
    }
    sort(ranked.begin(), ranked.end(),
    [](const auto& a, const auto& b) {
        return a.node.stake > b.node.stake;
    });

    int idx_r=0;

    std::queue<BroadcasterTree::TreeNode*> q;

    BroadcasterTree::TreeNode fake;
    fake.node.name.container="fake";
    BroadcasterTree::TreeNode root=fake;
    q.push(&root);
    // idx_r++;

    while (!q.empty() && idx_r<ranked.size()) {
        BroadcasterTree::TreeNode* cur = q.front();
        q.pop();



        for (int i = 0; i < 2; i++) {

            if(idx_r>=ranked.size())
                break;

            auto r=ranked[idx_r++];
            // const std::string name = r.name;
            // auto it = nodes.find(ip);
            // if (it == nodes.end()) continue; // защитный случай
            cur->children.emplace_back(r);   // копия в дерево
            BroadcasterTree::TreeNode* childPtr = &cur->children.back(); // стабильный адрес
            // nodes.erase(it);                          // удаляем из map
            q.push(childPtr);                         // добавляем в очередь
        }
    }
    return root;
}
// nlohmann::json BroadcasterTree::TreeNode::to_json() const
// {
//     nlohmann::json j;
//     j["name"] = meta.name;
//     j["stake"] = meta.stake.to_decimal_string();
//     j["children"] = nlohmann::json::array();
//     for (const auto& child : children) {
//         j["children"].push_back(child.to_json());
//     }
//     return j;
// }
// std::vector<uint8_t>BroadcasterTree::TreeNode::to_mpack() const
//     {
//     nlohmann::json j;
//     j["n"] = meta.name;
//     j["s"] = meta.stake.to_decimal_string();
//     j["c"] = nlohmann::json::array();
//     for (const auto& child : children) {
//         j["c"].push_back(child.to_json());
//     }
//     return nlohmann::json::to_msgpack(j);

// }


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