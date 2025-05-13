#include "mba.h"

uint64_t obfuscate_mba_64(uint64_t imm) {
    uint64_t mask = rand() | 1;     // Random mask
    uint64_t x = (imm ^ mask);
    // Result: ((x ^ mask) & 0xFFFFFFFF) == imm
    return x;
}

uint32_t obfuscate_mba_32(uint32_t imm) {
    uint32_t mask = rand() | 1;     // Random mask
    uint32_t x = (imm ^ mask);
    // Result: ((x ^ mask) & 0xFFFFFFFF) == imm
    return x;
}