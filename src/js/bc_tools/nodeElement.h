#pragma once
#include "blake2bHasher.h"
// #include "bigint.h"
#include "NODE_id.h"
struct NodeElement {
    NODE_id name;
    uint64_t stake_A;
    std::string ip;
    void hash(Blake2bHasher&h)
    {
        h.update(name.container);
        h.update(std::to_string(stake_A));
        h.update(ip);
    }
};
