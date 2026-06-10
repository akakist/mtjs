#pragma once
#include "blake2bHasher.h"
#include "bigint.h"
#include "NODE_id.h"
struct NodeElement {
    NODE_id name;
    BigInt stake_A;
    std::string ip;
    int missed_rounds=0;
    void hash(Blake2bHasher&h)
    {
        h.update(name.container);
        h.update(stake_A.toString());
        h.update(ip);
        h.update(std::to_string(missed_rounds));
    }
};
