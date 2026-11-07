#pragma once
static constexpr int MAX_CALL_DEPTH = 64;
static constexpr int MAX_SERIALIZE_DEPTH = 32;
static constexpr size_t MAX_GRANULE_SIZE = 10 * 1024;
static constexpr size_t MAX_CONTRACT_STACK = 64 * 1024;
static constexpr int64_t MAX_GRANULE_TTL = 10LL * 365 * 24 * 3600;   // 315360000 (10 лет)
static constexpr int64_t MAX_VIEW_TIMEOUT_SEC = 30;   // макс лимит view (защита от while(true))