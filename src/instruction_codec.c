#include <string.h>
#include <stdio.h>  
#include "engine.h"
#include "isa.h"  

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
        **new**
        xor reg, imm
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

        uint8_t reg = opcode - 0xB8;
        if (out->rex & 0x01)  // REX.B
            reg |= 0x08;
        out->op1 = reg;

        out->imm = *((uint32_t*)&code[offset]);
        offset += 8;  // corrent size of imm
        out->size = offset;
        return offset;
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

    // mov [rsp], imm32 — C7 04 24 <imm32>
    else if (opcode == 0xC7 && code[offset] == 0x04 && code[offset + 1] == 0x24) {
        offset += 2; // skip ModRM and SIB
        out->opcode = 0xC7;
        out->operand_type = OPERAND_MEM | OPERAND_IMM;
        out->op1 = RSP_REG;
        out->imm = *((uint32_t*)&code[offset]);
        offset += 4;
        out->size = offset;
        return offset;
    }

    // xor r/m64, imm32
    else if (opcode == 0x81) {
        uint8_t modrm = code[offset++];
        uint8_t reg_field = (modrm >> 3) & 0x07;    

        if (reg_field == 6) { // /6 = XOR
            uint8_t rm = modrm & 0x07;

            if (out->rex & 0x01) rm |= 0x08;

            out->opcode = 0x81;
            out->operand_type = OPERAND_REG | OPERAND_IMM;
            out->op1 = rm;
            out->imm = *((uint32_t*)&code[offset]);
            offset += 4;
            out->size = offset;
            return offset;
        }
    }

    // ADD r/m64, r64 -> 01 /r
    else if (opcode == 0x01) {
        uint8_t modrm = code[offset++];
        uint8_t reg = (modrm >> 3) & 0x07;
        uint8_t rm = modrm & 0x07;

        if (out->rex & 0x04) reg |= 0x08; // REX.R
        if (out->rex & 0x01) rm  |= 0x08; // REX.B

        out->opcode = 0x01;
        out->operand_type = OPERAND_REG | OPERAND_REG;
        out->op1 = rm;
        out->op2 = reg;
        out->size = offset;
        return offset;
    }

    // SUB r/m64, r64 → 29 /r
    else if (opcode == 0x29) {
        uint8_t modrm = code[offset++];
        uint8_t reg = (modrm >> 3) & 0x07;
        uint8_t rm = modrm & 0x07;

        if (out->rex & 0x04) reg |= 0x08; // REX.R
        if (out->rex & 0x01) rm  |= 0x08; // REX.B

        out->opcode = 0x29;
        out->operand_type = OPERAND_REG | OPERAND_REG;
        out->op1 = rm;   // destination
        out->op2 = reg;  // source
        out->size = offset;
        return offset;
    }

    // Short jumps (Jcc rel8)
    else if (opcode >= 0x70 && opcode <= 0x7F) {
        out->opcode = opcode;
        out->operand_type = OPERAND_IMM;
        out->imm = (int8_t)code[offset++];      // Signed 8-bit offset
        out->size = offset + 1;
        return out->size;
    }

    // Near jumps (0F 8x rel32)
    else if (opcode == 0x0F && (code[offset] >= 0x80 && code[offset] <= 0x8F)) {
        uint8_t jcc = code[offset++];
        out->opcode = 0x0F00 | jcc;
        out->operand_type = OPERAND_IMM;
        out->imm = *((int32_t*)&code[offset]);
        offset += 4;
        out->size = offset;
        return out->size;
    }

    // TEST r/m8, r8
    else if (opcode == 0x84) {
        uint8_t modrm = code[offset++];
        uint8_t reg = (modrm >> 3) & 0x07;
        uint8_t rm = (modrm & 0x07);

        out->opcode = 0x84;
        out->operand_type = OPERAND_REG | OPERAND_REG;
        out->op1 = rm;
        out->op2 = reg;
        out->size = offset;
        return offset;
    }

    // SETcc r8 → 0F 90 + cc /r
    else if (opcode == 0x0F && (code[offset] >= 0x90 && code[offset] <= 0x9F)) {
        uint8_t setcc = code[offset++];
        uint8_t modrm = code[offset++];

        if ((modrm & 0xC0) != 0xC0) return -1; // must be register form

        uint8_t reg = modrm & 0x07;

        out->opcode = 0x0F00 | setcc;
        out->operand_type = OPERAND_REG;
        out->op1 = reg;  // 8-bit register (e.g. AL_REG)
        out->size = offset;
        return offset;
    }

    // Unknown/unsupported instruction
    return -1;
}

/*
*
*
*
*
*
*
*
*
*
*
======================================== ENCODING ========================================
*
*
*
*
*
*
*
*
*
*
*/

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
        uint64_t imm64 = (uint64_t)(uint32_t)(inst->imm); // zero-extend
        memcpy(&out[offset], &imm64, 8);
        offset += 8;
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
    else if (inst->opcode == 0xC7 && inst->operand_type == (OPERAND_MEM | OPERAND_IMM) && inst->op1 == RSP_REG) {
        out[offset++] = 0xC7;        // Opcode
        out[offset++] = 0x04;        // ModRM byte: mod=00, reg=000, r/m=100 (SIB)
        out[offset++] = 0x24;        // SIB byte: scale=00, index=100 (none), base=100 (rsp)
        *((uint32_t*)&out[offset]) = inst->imm; // imm32
        offset += 4;
        return offset;
    }

    // XOR r/m64, imm32 -> REX.W + 81 /6 + imm32
    else if (inst->opcode == 0x81 && inst->operand_type == (OPERAND_REG | OPERAND_IMM)) {

        out[offset++] = 0x81;

        // ModRM: mod=11 (register), reg=6 (XOR), r/m = reg
        uint8_t modrm = 0xC0 | (6 << 3) | (inst->op1 & 0x07);
        out[offset++] = modrm;

        *((uint32_t*)&out[offset]) = inst->imm;
        offset += 4;

        return offset;
    }

    // ADD r/m64, r64 -> 01 /r
    else if (inst->opcode == 0x01 && inst->operand_type == (OPERAND_REG | OPERAND_REG)) {
        out[offset++] = 0x01;

        uint8_t modrm = 0xC0 | ((inst->op2 & 0x07) << 3) | (inst->op1 & 0x07);
        out[offset++] = modrm;

        return offset;
    }

    // SUB r/m64, r64 → 29 /r
    else if (inst->opcode == 0x29 && inst->operand_type == (OPERAND_REG | OPERAND_REG)) {
        out[offset++] = 0x29;

        uint8_t modrm = 0xC0 | ((inst->op2 & 0x07) << 3) | (inst->op1 & 0x07);
        out[offset++] = modrm;

        return offset;
    }

    // Short jumps
    else if ((inst->opcode >= 0x70 && inst->opcode <= 0x7F) && inst->operand_type == OPERAND_IMM) {
        out[offset++] = inst->opcode;
        out[offset++] = (int8_t)inst->imm;
        return offset;
    }

    // Near jumps
    else if ((inst->opcode & 0xFF00) == 0x0F00 && inst->operand_type == OPERAND_IMM) {
        out[offset++] = 0x0F;
        out[offset++] = inst->opcode & 0xFF;
        *((int32_t*)&out[offset]) = inst->imm;
        offset += 4;
        return offset;
    }

    // TEST r/m8, r8 -> 84 /r
    else if (inst->opcode == 0x84 && inst->operand_type == (OPERAND_REG | OPERAND_REG)) {
        out[offset++] = 0x84;
        uint8_t modrm = 0xC0 | ((inst->op2 & 0x07) << 3) | (inst->op1 & 0x07);
        out[offset++] = modrm;
        return offset;
    }

    // SETcc r8 → 0F 90–9F /r
    else if ((inst->opcode & 0xFF00) == 0x0F00 &&
        (inst->opcode & 0xFF) >= 0x90 &&
        (inst->opcode & 0xFF) <= 0x9F &&
        inst->operand_type == OPERAND_REG)
    {
        out[offset++] = 0x0F;
        out[offset++] = inst->opcode & 0xFF;
        out[offset++] = 0xC0 | (inst->op1 & 0x07);
        return offset;
    }
    
    // Unknown/unsupported instruction
    return -1;
}