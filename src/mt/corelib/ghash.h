#pragma once
#include <cstdint>
constexpr uint32_t ghash(const char* str) {
    uint32_t hash = 2166136261u;  // FNV offset basis
    while (*str) {
        hash ^= static_cast<uint32_t>(*str++);
        hash *= 16777619u;  // FNV prime
    }
    return hash;
}
