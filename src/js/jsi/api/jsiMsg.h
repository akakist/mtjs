#pragma once
#include <string>
// ============================================================
// jsiMsg
// ============================================================
struct jsiMsg {
    std::string sender;
    int64_t value = 0;
    int64_t gasLimit = 0;
    int64_t gasUsed = 0;
    int64_t blockTimestamp = 0;
    int64_t blockHeight = 0;
    int64_t nonce = 0;
    int64_t chainId = 0;
    std::string txHash;
    int64_t txIndex = 0;
};
