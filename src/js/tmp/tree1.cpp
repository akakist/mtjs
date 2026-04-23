#include <bits/stdc++.h>
#include <nlohmann/json.hpp>
using namespace std;
using json = nlohmann::json;

struct Node {
    string ip;   // ip:port — ID
    double lat, lon;
};

class TreeNode {
public:
    Node meta;
    vector<TreeNode> children; // копии узлов

    TreeNode(const Node& n) : meta(n) {}

    json to_json() const {
        json j;
        j["ip"] = meta.ip;
        j["lat"] = meta.lat;
        j["lon"] = meta.lon;
        j["children"] = json::array();
        for (auto& child : children) {
            j["children"].push_back(child.to_json());
        }
        return j;
    }
};

// Хаверсин
double haversine(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371.0;
    auto toRad = [](double d){ return d * M_PI / 180.0; };
    double dLat = toRad(lat2 - lat1);
    double dLon = toRad(lon2 - lon1);
    double a = sin(dLat/2)*sin(dLat/2) +
               cos(toRad(lat1))*cos(toRad(lat2)) *
               sin(dLon/2)*sin(dLon/2);
    double c = 2 * asin(sqrt(a));
    return R * c;
}

// Построение дерева: root + map узлов
TreeNode buildTree(map<string,Node>& nodes, const string& rootIp) {
    // берём корень
    Node rootNode = nodes[rootIp];
    nodes.erase(rootIp);
    TreeNode root(rootNode);

    queue<TreeNode*> q;
    q.push(&root);

    while (!q.empty() && !nodes.empty()) {
        TreeNode* cur = q.front(); q.pop();

        // ранжируем оставшиеся по расстоянию
        vector<pair<double,string>> ranked;
        for (auto& kv : nodes) {
            double d = haversine(cur->meta.lat, cur->meta.lon,
                                 kv.second.lat, kv.second.lon);
            ranked.push_back({d, kv.first});
        }
        sort(ranked.begin(), ranked.end());

        int take = min(2,(int)ranked.size());
        for (int i=0;i<take;i++) {
            string ip = ranked[i].second;
            Node n = nodes[ip];
            nodes.erase(ip); // удаляем из map
            cur->children.emplace_back(n); // копируем в дерево
            q.push(&cur->children.back());
        }
    }
    return root;
}

int main() {
    map<string,Node> nodes = {
        {"10.0.0.1:8000",{"10.0.0.1:8000",55.75,37.61}}, // Москва
        {"192.168.1.10:9000",{"192.168.1.10:9000",51.51,-0.13}}, // Лондон
        {"203.0.113.5:8080",{"203.0.113.5:8080",48.85,2.35}},    // Париж
        {"2001:db8::1:443",{"2001:db8::1:443",40.71,-74.01}},    // Нью-Йорк
        {"172.16.0.2:7000",{"172.16.0.2:7000",35.68,139.69}},    // Токио
        {"198.51.100.7:6000",{"198.51.100.7:6000",34.05,-118.24}}, // Лос-Анджелес
        {"2001:db8::2:8443",{"2001:db8::2:8443",52.37,4.89}},    // Амстердам
        {"203.0.113.9:5000",{"203.0.113.9:5000",41.90,12.49}},   // Рим
        {"192.0.2.11:3000",{"192.0.2.11:3000",39.90,116.40}},    // Пекин
        {"2001:db8::3:8081",{"2001:db8::3:8081",19.43,-99.13}},  // Мехико
        {"198.51.100.12:5050",{"198.51.100.12:5050",37.77,-122.42}} // Сан-Франциско
    };

    string rootIp = "10.0.0.1:8000"; // Москва — корень
    TreeNode root = buildTree(nodes, rootIp);

    json j = root.to_json();
    cout << j.dump(4) << endl;
}
