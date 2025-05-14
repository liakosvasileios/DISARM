#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <stdint.h>


/*
    
*/

struct Instruction{
    uint16_t opcode;             // Unique internal representation of the instrution opcode
    uint8_t operand_type;       // Operand types byte
    uint8_t op1;                // First operand (could be register ID, memory base reg, etc.)
    uint8_t op2;                // Second operand (same format as the first operand)
    uint32_t imm;               // Immediate or displacement value
    uint8_t size;               // Total size in bytes when reassembled
    uint8_t rex;                // REX prefix byte (0x40-0x4F for x86_64)

};


#endif  // INSTRUCTION_H