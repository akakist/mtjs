#pragma once
#include <stdint.h>
#include <vector>
#include <algorithm>

inline int64_t get_tariff(int64_t ttl_seconds) {
    if (ttl_seconds == -1) return 15;           // бессрочно
    
    if (ttl_seconds <= 86400) return 1;         // 1 день
    if (ttl_seconds <= 604800) return 2;        // 7 дней
    if (ttl_seconds <= 2592000) return 3;       // 30 дней
    if (ttl_seconds <= 7776000) return 4;       // 90 дней
    if (ttl_seconds <= 15552000) return 5;      // 180 дней
    if (ttl_seconds <= 31536000) return 6;      // 1 год
    if (ttl_seconds <= 94608000) return 10;     // 3 года
    
    return 15;  // больше 3 лет -> приравниваем к бессрочному? или 15?
}
struct TtlRule {
    int max_ttl;   // максимальный TTL (в секундах)
    int tariff;    // тариф
};
static std::vector<TtlRule> rules = {
    {604800, 1},      // 1 неделя
    {1209600, 1},     // 2 недели
    {2592000, 2},     // 3-4 недели
    {5184000, 2},     // 5-6 недель
    {7776000, 3},     // 2-3 месяца
    {15552000, 4},    // 3-6 месяцев
    {31536000, 5},    // 6-12 месяцев
    {63072000, 6},    // 1-2 года
    {94608000, 7},    // 2-3 года
};

int get_tariff2(int ttl_seconds) {
    auto it = std::lower_bound(rules.begin(), rules.end(), ttl_seconds,
        [](const TtlRule& rule, int ttl) {
            return rule.max_ttl < ttl;  // ищем первый, у которого max_ttl >= ttl
        });
    
    if (it == rules.end()) {
        // TTL больше 3 лет -> бессрочный тариф
        return -1;
    }
    
    return it->tariff;
}