#include "IDatabase.h"
uint64_t fnv1a_64(const void* buffer, size_t len) 
{
    const unsigned char* data = (const unsigned char*)buffer;
    uint64_t hash = 0xCBF29CE484222325ULL; // FNV-64 offset basis
    
    for (size_t i = 0; i < len; i++) {
        hash ^= data[i];
        hash *= 0x100000001B3ULL; // FNV-64 prime
    }
    
    return hash;
}
