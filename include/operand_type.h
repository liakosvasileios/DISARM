#ifndef OPERAND_TYPE_H
#define OPERAND_TYPE_H

#include <stdint.h>

enum OperandType {
    OPERAND_NONE    = 0x00,     // No operand
    OPERAND_REG     = 0x01,     // Register
    OPERAND_IMM     = 0x02,     // Immediate
    OPERAND_MEM     = 0x04,     // Memory access
    OPERAND_RIPREL  = 0x08,     // RIP-relative addressing
    OPERAND_DIPL    = 0x10,     // Displacement-based memory
    OPERAND_SIB     = 0x20,     // SIB-byte based memory
    OPERAND_PTR     = 0x40,     // Far pointer operand (not common, for completeness)
};

#endif  // OPERAND_TYPE_H