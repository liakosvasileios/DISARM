#include <string.h>
#include <stdio.h>
#include "engine.h"
#include "isa.h"

int decode_instruction(const uint8_t *code, struct Instruction *out) {
    int offset = 0;
    memset(out, 0, sizeof(struct Instruction));

    uint8_t rex = 0;

    // Step 1: Optional REX prefix
    if ((code[offset] & 0xF0) == 0x40) {
        rex = code[offset];
        offset++;
    }

    // Step 2: Opcode
    uint8_t opcode = code[offset++];

    // Step 3: MOV r32/r64, imm
    if ((opcode >= 0xB8) && (opcode <= 0xBF)) {
        uint8_t reg = opcode - 0xB8;
        if (rex & 0x01) reg |= 0x08; // REX.B

        out->opcode = 0xB8;
        out->operand_type = OPERAND_REG | OPERAND_IMM;
        out->op1 = reg;
        out->rex = rex;

        if ((rex & 0x08) == 0x08) {
            out->imm = *((uint64_t *)&code[offset]);
            offset += 8;
        } else {
            out->imm = *((uint32_t *)&code[offset]);
            offset += 4;
        }

        out->size = offset;
        return offset;
    }

    // Handle other opcodes
    switch (opcode) {
        case OPCODE_MOV_MEM_REG: {
            FETCH_MODRM();
            APPLY_REX_BITS();
            out->opcode = OPCODE_MOV_MEM_REG;
            out->operand_type = OPERAND_MEM | OPERAND_REG;
            out->op1 = rm;
            out->op2 = reg;
            out->rex = rex;
            out->size = offset;
            return offset;
        }

        case OPCODE_MOV_REG_MEM: {
            FETCH_MODRM();
            APPLY_REX_BITS();
            out->opcode = OPCODE_MOV_REG_MEM;
            out->operand_type = OPERAND_REG | OPERAND_MEM;
            out->op1 = reg;
            out->op2 = rm;
            out->rex = rex;
            out->size = offset;
            return offset;
        }

        case OPCODE_ADD_RAX_IMM32: {
            out->opcode = OPCODE_ADD_RAX_IMM32;
            out->operand_type = OPERAND_REG | OPERAND_IMM;
            out->op1 = RAX_REG;
            out->imm = *((uint32_t*)&code[offset]);
            offset += 4;
            out->rex = rex;
            out->size = offset;
            return offset;
        }

        case OPCODE_SUB_RAX_IMM32: {
            out->opcode = OPCODE_SUB_RAX_IMM32;
            out->operand_type = OPERAND_REG | OPERAND_IMM;
            out->op1 = RAX_REG;
            out->imm = *((uint32_t*)&code[offset]);
            offset += 4;
            out->rex = rex;
            out->size = offset;
            return offset;
        }

        case OPCODE_XOR_REG_REG: {
            FETCH_MODRM();
            APPLY_REX_BITS();
            out->opcode = OPCODE_XOR_REG_REG;
            out->operand_type = OPERAND_REG | OPERAND_REG;
            out->op1 = rm;
            out->op2 = reg;
            out->rex = rex;
            out->size = offset;
            return offset;
        }

        case OPCODE_PUSH_IMM32: {
            out->opcode = OPCODE_PUSH_IMM32;
            out->operand_type = OPERAND_IMM;
            out->imm = *((uint32_t*)&code[offset]);
            offset += 4;
            out->rex = rex;
            out->size = offset;
            return offset;
        }

        case OPCODE_XCHG_REG_REG: {
            FETCH_MODRM();
            APPLY_REX_BITS();
            out->opcode = OPCODE_XCHG_REG_REG;
            out->operand_type = OPERAND_REG | OPERAND_REG;
            out->op1 = rm;
            out->op2 = reg;
            out->rex = rex;
            out->size = offset;
            return offset;
        }

        case OPCODE_MOV_MEM_IMM32: {
            if (code[offset] == 0x04 && code[offset + 1] == 0x24) {
                offset += 2;
                out->opcode = OPCODE_MOV_MEM_IMM32;
                out->operand_type = OPERAND_MEM | OPERAND_IMM;
                out->op1 = RSP_REG;
                out->imm = *((uint32_t*)&code[offset]);
                offset += 4;
                out->rex = rex;
                out->size = offset;
                return offset;
            }
            break;
        }

        case OPCODE_XOR_REG_IMM32: {
            FETCH_MODRM();
            if (((modrm >> 3) & 0x07) == 6) {
                uint8_t rm = modrm & 0x07;
                if (rex & 0x01) rm |= 0x08;
                out->opcode = OPCODE_XOR_REG_IMM32;
                out->operand_type = OPERAND_REG | OPERAND_IMM;
                out->op1 = rm;
                out->imm = *((uint32_t*)&code[offset]);
                offset += 4;
                out->rex = rex;
                out->size = offset;
                return offset;
            }
            break;
        }

        case OPCODE_ADD_REG_REG: {
            FETCH_MODRM();
            APPLY_REX_BITS();
            out->opcode = OPCODE_ADD_REG_REG;
            out->operand_type = OPERAND_REG | OPERAND_REG;
            out->op1 = rm;
            out->op2 = reg;
            out->rex = rex;
            out->size = offset;
            return offset;
        }

        case OPCODE_SUB_REG_REG: {
            FETCH_MODRM();
            APPLY_REX_BITS();
            out->opcode = OPCODE_SUB_REG_REG;
            out->operand_type = OPERAND_REG | OPERAND_REG;
            out->op1 = rm;
            out->op2 = reg;
            out->rex = rex;
            out->size = offset;
            return offset;
        }

        case OPCODE_TEST_REG8_REG8: {
            out->opcode = OPCODE_TEST_REG8_REG8;
            out->operand_type = OPERAND_REG | OPERAND_REG;
            out->op1 = AL_REG;
            out->op2 = AL_REG;
            out->rex = rex;
            out->size = offset;
            return offset;
        }
    }

    // Jcc short
    if (opcode >= OPCODE_JCC_SHORT_MIN && opcode <= OPCODE_JCC_SHORT_MAX) {
        out->opcode = opcode;
        out->operand_type = OPERAND_IMM;
        out->imm = (int8_t)code[offset++];
        out->rex = rex;
        out->size = offset;
        return offset;
    }

    // Jcc near
    if (opcode == OPCODE_JCC_NEAR) {
        uint8_t ext = code[offset++];
        if (ext >= 0x80 && ext <= 0x8F) {
            out->opcode = 0x0F00 | ext;
            out->operand_type = OPERAND_IMM;
            out->imm = *((int32_t*)&code[offset]);
            offset += 4;
            out->rex = rex;
            out->size = offset;
            return offset;
        } else if (ext >= 0x90 && ext <= 0x9F) {
            uint8_t modrm = code[offset++];
            if ((modrm & 0xC0) == 0xC0) {
                out->opcode = 0x0F00 | ext;
                out->operand_type = OPERAND_REG;
                out->op1 = modrm & 0x07;
                out->rex = rex;
                out->size = offset;
                return offset;
            }
        }
    }

    // CALL rel32
    if (opcode == OPCODE_CALL_REL32) {
        out->opcode = OPCODE_CALL_REL32;
        out->operand_type = OPERAND_IMM;
        out->imm = *((int32_t*)&code[offset]);
        offset += 4;
        out->rex = rex;
        out->size = offset;
        return offset;
    }

    // CALL [r64] (FF /2)
    if (opcode == 0xFF) {
        uint8_t modrm = code[offset++];
        if (((modrm >> 3) & 0x07) == 2 && ((modrm >> 6) & 0x03) == 0x03) {
            out->opcode = OPCODE_CALL_R64;
            out->operand_type = OPERAND_REG;
            out->op1 = modrm & 0x07;
            if (rex & 0x01) out->op1 |= 0x08;
            out->rex = rex;
            out->size = offset;
            return offset;
        }
    }

    return -1; // Unknown instruction
}
