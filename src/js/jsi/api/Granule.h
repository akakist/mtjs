#pragma once
#include <string>
struct Granule {
    std::string value;
    int64_t create_time = 0;
    int64_t ttl = 0;
};
