#include <stdint.h>
#include <string.h>
#include <stdio.h>  

#include "registers.h"
#include "operand_type.h"
#include "engine.h"
#include "instruction.h"  

/*
    Supported instrutions:
        mov reg, imm
        mov r/m64, r64
        mov r32, r/m32
        add RAX, imm
        sub rax, imm
        xor reg, reg
        push imm32
        xchg reg, reg
        mov [reg], imm
        **new**s
*/
int decode_instruction(const uint8_t *code, struct Instruction *out) {
    int offset = 0;

    // Clear the instruction structure
    memset(out, 0, sizeof(struct Instruction));

    // Check for REX prefix (0x40 - 0x4F)
    out->rex = 0;
    if ((code[0] & 0xF0) == 0x40) {
        out->rex = code[0];
        offset++;
    }

    uint8_t opcode = code[offset++];

    // MOV reg, imm32/imm64: B8+rd
    if (opcode >= 0xB8 && opcode <= 0xBF) {
        out->opcode = 0xB8;
        out->operand_type = OPERAND_REG | OPERAND_IMM;

        uint8_t reg = opcode - 0xB8;   // Encoded register number

        if (out->rex & 0x01)  // REX.B bit extends reg
            reg |= 0x08;

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

        if (out->rex & 0x04) reg |= 0x08; // REX.R
        if (out->rex & 0x01) rm  |= 0x08; // REX.B

        out->opcode = 0x89;
        out->operand_type = OPERAND_MEM | OPERAND_REG;
        out->op1 = rm;
        out->op2 = reg;
        out->size = offset;
        return offset;
    }

    // MOV r32, r/m32: 8B /r
    else if (opcode == 0x8B) {
        uint8_t modrm = code[offset++];
        uint8_t reg = (modrm >> 3) & 0x07;  
        uint8_t rm = modrm & 0x07;

        if (out->rex & 0x04) reg |= 0x08; // REX.R
        if (out->rex & 0x01) rm  |= 0x08; // REX.B

        out->opcode = 0x8B;
        out->operand_type = OPERAND_REG | OPERAND_MEM;
        
        out->op1 = reg;
        out->op2 = rm;
        out->size = offset;
        return offset;
    }

    // ADD RAX, imm32: 05 id
    else if (opcode == 0x05) {
        out->opcode = 0x05;
        out->operand_type = OPERAND_REG | OPERAND_IMM;
        out->op1 = RAX_REG;     // Hardcoded because 0x05 implies RAX

        out->imm = *((uint32_t*)&code[offset]);
        out->size = offset + 4;
        return out->size;
    }

    // sub rax, imm32: 2D id
    else if (opcode == 0x2D) {
        
        out->opcode = 0x2D;
        out->operand_type = OPERAND_REG | OPERAND_IMM;
        out->op1 = RAX_REG;     // Hardcoded because 0x2D implies RAX
        out->imm = *((uint32_t*)&code[offset]);
        out->size = offset + 4;
        return out->size;
    }

    // xor reg, reg: 31 /r
    else if (opcode == 0x31) {
        uint8_t modrm = code[offset++];
        uint8_t reg = (modrm >> 3) & 0x07;
        uint8_t rm = modrm & 0x07;

        if (out->rex & 0x04) reg |= 0x08; // REX.R
        if (out->rex & 0x01) rm  |= 0x08; // REX.B

        out->opcode = 0x31;
        out->operand_type = OPERAND_REG | OPERAND_REG;
        out->op1 = rm;
        out->op2 = reg;
        out->size = offset;
        return out->size;
    }

    // push imm32: 68 id
    else if (opcode == 0x68) {
        out->opcode = 0x68;
        out->operand_type = OPERAND_IMM;
        out->imm = *((uint32_t*)&code[offset]);
        out->size = offset + 4;
        return out->size;
    }

    // xchg reg, reg: 87 /r
    else if (opcode == 0x87) {
        uint8_t modrm = code[offset++];
        uint8_t reg = (modrm >> 3) & 0x07;
        uint8_t rm = modrm & 0x07;

        if (out->rex & 0x04) reg |= 0x08;
        if (out->rex & 0x01) rm  |= 0x08;

        out->opcode = 0x87;
        out->operand_type = OPERAND_REG | OPERAND_REG;
        out->op1 = rm;
        out->op2 = reg;
        out->size = offset;
        return out->size;
    }

    // Unknown/unsupported instruction
    return -1;
}

// Encodes an Instruction into x86_64 machine code.
// Returns the number of bytes written or -1 on error.
int encode_instruction(const struct Instruction *inst, uint8_t *out) {
    int offset = 0;

    // Apply REX prefix if present
    if (inst->rex) {
        out[offset++] = inst->rex;
    } else if ((inst->op1 >= 8 || inst->op2 >= 8) || 
              (inst->opcode == 0xB8 || inst->opcode == 0x05)) {
        // Default REX.W prefix for 64-bit mode if needed
        uint8_t rex = 0x48; // REX.W prefix for 64-bit
        
        if (inst->op1 >= 8)
            rex |= 0x01; // REX.B
        if (inst->op2 >= 8)
            rex |= 0x04; // REX.R
            
        out[offset++] = rex;
    }

    // mov reg, imm32/imm64: B8+rd - NOTE: in 64-bit mode, immediate is sign-extended
    if (inst->opcode == 0xB8 && inst->operand_type == (OPERAND_REG | OPERAND_IMM)) {
        out[offset++] = 0xB8 + (inst->op1 & 0x07); // Base opcode + lower 3 bits of reg
        *((uint32_t*)&out[offset]) = inst->imm;    // 32-bit immediate (sign-extended to 64)
        offset += 4;
        return offset;
    }

    // MOV r/m64, r64 (89 /r)
    else if (inst->opcode == 0x89 && inst->operand_type == (OPERAND_MEM | OPERAND_REG)) {
        out[offset++] = 0x89;      // Opcode

        // Encode ModR/M byte: mod=11 for register-direct (simple case)
        out[offset++] = 0xC0 | ((inst->op2 & 0x07) << 3) | (inst->op1 & 0x07);
        return offset;
    }

    // MOV r64, r/m64 (8B /r)
    else if (inst->opcode == 0x8B && inst->operand_type == (OPERAND_REG | OPERAND_MEM)) {
        out[offset++] = 0x8B;
        out[offset++] = 0xC0 | ((inst->op1 & 0x07) << 3) | (inst->op2 & 0x07);
        return offset;
    }

    // ADD RAX, imm32 (05 id)
    else if (inst->opcode == 0x05 && inst->operand_type == (OPERAND_REG | OPERAND_IMM)) {
        out[offset++] = 0x05;  // ADD RAX, imm32
        *((uint32_t*)&out[offset]) = inst->imm; 
        offset += 4;
        return offset;
    }

    // SUB RAX, imm32: 2D id
    else if (inst->opcode == 0x2D && inst->operand_type == (OPERAND_REG | OPERAND_IMM)) {
        out[offset++] = 0x2D;
        *((uint32_t*)&out[offset]) = inst->imm;
        offset += 4;
        return offset;
    }

    // XOR reg, reg: 31 /r
    else if (inst->opcode == 0x31 && inst->operand_type == (OPERAND_REG | OPERAND_REG)) {
        out[offset++] = 0x31;
        out[offset++] = 0xC0 | ((inst->op2 & 0x07) << 3) | (inst->op1 & 0x07);
        return offset;
    }

    // PUSH imm32: 68 id
    else if (inst->opcode == 0x68 && inst->operand_type == OPERAND_IMM) {
        out[offset++] = 0x68;
        *((uint32_t*)&out[offset]) = inst->imm;
        offset += 4;
        return offset;
    }

    // XCHG reg, reg: 87 /r
    else if (inst->opcode == 0x87 && inst->operand_type == (OPERAND_REG | OPERAND_REG)) {
        out[offset++] = 0x87;
        out[offset++] = 0xC0 | ((inst->op2 & 0x07) << 3) | (inst->op1 & 0x07);
        return offset;
    }

    // MOV [rsp], imm32: C7 04 24 <imm32>
    else if (inst->opcode == 0xC7 &&
        inst->operand_type == (OPERAND_MEM | OPERAND_IMM) &&
        inst->op1 == RSP_REG) {
    out[offset++] = 0xC7;        // Opcode
    out[offset++] = 0x04;        // ModRM byte: mod=00, reg=000, r/m=100 (SIB)
    out[offset++] = 0x24;        // SIB byte: scale=00, index=100 (none), base=100 (rsp)
    *((uint32_t*)&out[offset]) = inst->imm; // imm32
    offset += 4;
    return offset;
    }

    // Unknown/unsupported instruction
    return -1;
}