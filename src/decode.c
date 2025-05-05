#include <stdint.h>
#include <string.h>

#include "registers.h"
#include "operand_type.h"
#include "engine.h"
#include "instruction.h"  

/*
*
*  Implement support for a small set of simple instructions:
    MOV	    B8+r imm32	        MOV r32, imm32
    89 /r	MOV r/m32, r32	
    8B /r	MOV r32, r/m32	
    05 id	ADD EAX, imm32
*/
int decode_instruction(const uint8_t *code, struct Instruction *out) {
    int offset = 0;

    // Check for REX prefix (0x40 - 0x4F)
    uint8_t rex = 0;
    if ((code[0] & 0xF0) == 0x40) {
        rex = code[0];
        offset++;
    }

    uint8_t opcode = code[offset++];

    // MOV reg, imm32/imm64: B8+rd
    if (opcode >= 0xB8 && opcode <= 0xBF) {
        out->opcode = 0xB8;
        out->operand_type = OPERAND_REG | OPERAND_IMM;

        uint8_t reg = opcode - 0xB8;   // Encoded register number

        if (rex & 0x01)  // REX.B bit extends reg
            reg |= 0x08;

        out->op1 = reg;

        out->op1 = reg;
        out->imm = *((uint32_t*)&code[offset]);
        out->size = offset + 4;
        return out->size;
    }

    // MOV r/m64, r64: 89 /r
    else if (opcode == 0x89) {
        uint8_t modrm = code[offset++];

        // Extract the 'reg' field (bits 3–5) — this typically encodes the source register
        uint8_t reg = (modrm >> 3) & 0x07;

        // Extract the 'r/m' field (bits 0–2) — this typically encodes the destination
        // It can refer to a register or memory, depending on the 'mod' bits (not decoded here)
        uint8_t rm = modrm & 0x07;

        if (rex & 0x04) reg |= 0x08; // REX.R
        if (rex & 0x01) rm  |= 0x08; // REX.B

        out->opcode = 0x89;
        out->operand_type = OPERAND_REG | OPERAND_MEM;
        out->op1 = rm;
        out->op2 = reg;
        out->size = offset;
        return offset;
    }

    // MOV r32, r/m32: 8B /r
    else if (opcode == 0x8B) {
        uint8_t modrm = code[1];
        uint8_t reg = (modrm >> 3) & 0x07;  
        uint8_t rm = modrm & 0x07;

        out->opcode = 0x8B;
        out->operand_type = OPERAND_REG | OPERAND_MEM;
        
        out->op1 = reg;
        out->op2 = rm;
        out->size = 2;
        return 2;
    }

    // ADD RAX, imm64: 05 id
    else if (opcode == 0x05) {
        out->opcode = 0x05;
        out->operand_type = OPERAND_REG | OPERAND_IMM;
        out->op1 = RAX_REG;     // Hardcoded because 0x05 implies RAX

        out->imm = *((uint32_t*)&code[offset]);
        out->size = offset + 4;
        return out->size;
    }

    // Unknown/unsupported instruction
    return -1;
}

// Encodes an Instruction into x86_64 machine code.
// Returns the number of bytes written or -1 on error.
int encode_instruction(const struct Instruction *inst, uint8_t *out) {

    // mov reg, imm32/imm64: B8+rd - NOTE: in 64-bit mode, immediate is sign-extended
    if (inst->opcode == 0xB8 && inst->operand_type == (OPERAND_REG | OPERAND_IMM)) {
        uint8_t rex = 0x48; // REX.W prefix for 64-bit
        if (inst->op1 >= 8) {
            rex |= 0x01; // Set REX.B if register index is extended
        }

        out[0] = rex;
        out[1] = 0xB8 + (inst->op1 & 0x07); // Base opcode + lower 3 bits of reg
        *((uint32_t*)&out[2]) = inst->imm;       // 32-bit immediate (sign-extended to 64)
        return 6;
    }

    // MOV r/m64, r64 (89 /r)
    else if (inst->opcode == 0x89 && inst->operand_type == (OPERAND_MEM | OPERAND_REG)) {
        uint8_t rex = 0x48; // 64-bit operand size

        if (inst->op2 >= 8) rex |= 0x04; // REX.R
        if (inst->op1 >= 8) rex |= 0x01; // REX.B

        out[0] = rex;
        out[1] = 0x89;

        // Encode ModR/M byte: mod=11 for register-direct (simple case)
        out[2] = 0xC0 | ((inst->op2 & 0x07) << 3) | (inst->op1 & 0x07);
        return 3;
    }

    // MOV r64, r/m64 (8B /r)
    else if (inst->opcode == 0x8B && inst->operand_type == (OPERAND_REG | OPERAND_MEM)) {
        uint8_t rex = 0x48;

        if (inst->op1 >= 8) rex |= 0x04; // REX.R
        if (inst->op2 >= 8) rex |= 0x01; // REX.B

        out[0] = rex;
        out[1] = 0x8B;
        out[2] = 0xC0 | ((inst->op1 & 0x07) << 3) | (inst->op2 & 0x07);
        return 3;
    }

    // ADD RAX, imm32 (05 id)
    else if (inst->opcode == 0x05 && inst->operand_type == (OPERAND_REG | OPERAND_IMM)) {
        out[0] = 0x48;  // REX.W for 64-bit
        out[1] = 0x05;  // ADD RAX, imm32
        *((uint32_t*)&out[2]) = inst->imm; 
        return 6;
    }

    

    // Unknown/unsupported instruction
    return -1;
}
