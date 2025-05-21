#include <string.h>
#include <stdio.h>
#include "engine.h"
#include "isa.h"

int decode_instruction(const uint8_t *code, struct Instruction *out) {
    int offset = 0;
    memset(out, 0, sizeof(*out));
    uint8_t rex = 0;

    // REX prefix
    if ((code[offset] & 0xF0) == 0x40) {
        rex = code[offset++];
    }

    uint8_t opcode = code[offset++];

    // MOV r64/32, imm
    if (opcode >= 0xB8 && opcode <= 0xBF) {
        uint8_t reg = opcode - 0xB8;
        if (rex & 0x01) reg |= 0x08;
        out->opcode      = OPCODE_MOV_REG_IMM64;
        out->operand_type = OPERAND_REG|OPERAND_IMM;
        out->op1         = reg;
        out->rex         = rex;
        if (rex & 0x08) {
            out->imm = *(uint64_t*)&code[offset]; offset += 8;
        } else {
            out->imm = *(uint32_t*)&code[offset]; offset += 4;
        }
        out->size = offset;
        return offset;
    }

    // Jcc short-form (1-byte opcode): 0x70–0x7F
    if (opcode >= 0x70 && opcode <= 0x7F) {
        out->opcode = opcode;
        out->operand_type = OPERAND_IMM;
        out->imm = (int8_t)code[offset++];  // 8-bit relative jump
        out->rex = rex;
        out->size = offset;
        return offset;
    }

    // simple call rel32
    if (opcode == OPCODE_CALL_REL32) {
        out->opcode      = OPCODE_CALL_REL32;
        out->operand_type = OPERAND_IMM;
        out->imm         = *(int32_t*)&code[offset]; offset += 4;
        out->rex         = rex;
        out->size        = offset;
        return offset;
    }

    switch (opcode) {
        case OPCODE_MOV_MEM_REG: {
            uint8_t modrm, mod, reg, rm;
            FETCH_MODRM(); 
            APPLY_REX_BITS();
            out->opcode       = OPCODE_MOV_MEM_REG;
            out->operand_type = OPERAND_MEM | OPERAND_REG;
            // handle SIB
            mod = (modrm >> 6) & 0x03;
            if (rm == 4 && mod != 3) {
                FETCH_SIB();
            } else {
                out->scale = 0;
                out->index = REG_INVALID;
                out->base = rm;
            }
            // displacement
            if (mod == 1) {
                out->disp = (int8_t)code[offset++];
            } else if (mod == 2 || (mod==0 && out->base==5)) {
                out->disp = *(int32_t*)&code[offset]; offset+=4;
                if (mod==0 && out->base==5) out->base=REG_INVALID;
            }
            out->op1 = out->base;
            out->op2 = reg;
            out->rex = rex;
            out->size= offset;
            return offset;
        }
        case OPCODE_MOV_REG_MEM: {
            uint8_t modrm, mod, reg, rm;
            FETCH_MODRM(); 
            APPLY_REX_BITS();
            out->opcode      = OPCODE_MOV_REG_MEM;
            out->operand_type = OPERAND_REG | OPERAND_MEM;
            mod = (modrm>>6)&0x03;
            if (rm == 4 && mod != 3) {
                FETCH_SIB();
            } else {
                out->scale = 0;
                out->index = REG_INVALID;
                out->base = rm;
            }
            if (mod == 1) {
                out->disp = (int8_t)code[offset++];
            } else if (mod == 2 || (mod==0 && out->base==5)) {
                out->disp = *(int32_t*)&code[offset]; offset+=4;
                if (mod==0 && out->base==5) out->base=REG_INVALID;
            }
            out->op1 = reg;
            out->op2 = out->base;
            out->rex = rex;
            out->size= offset;
            return offset;
        }
        case OPCODE_ADD_RAX_IMM32: {
            out->opcode      = OPCODE_ADD_RAX_IMM32;
            out->operand_type= OPERAND_REG|OPERAND_IMM;
            out->op1         = RAX_REG;
            out->imm         = *(uint32_t*)&code[offset]; offset+=4;
            out->rex         = rex;
            out->size        = offset;
            return offset;
        }
        case OPCODE_SUB_RAX_IMM32: {
            out->opcode      = OPCODE_SUB_RAX_IMM32;
            out->operand_type= OPERAND_REG|OPERAND_IMM;
            out->op1         = RAX_REG;
            out->imm         = *(uint32_t*)&code[offset]; offset+=4;
            out->rex         = rex;
            out->size        = offset;
            return offset;
        }
        case OPCODE_XOR_REG_REG: {
            uint8_t modrm, mod, reg, rm;
            FETCH_MODRM(); 
            APPLY_REX_BITS();
            out->opcode      = OPCODE_XOR_REG_REG;
            out->operand_type= OPERAND_REG|OPERAND_REG;
            out->op1         = rm;
            out->op2         = reg;
            out->rex         = rex;
            out->size        = offset;
            return offset;
        }
        case OPCODE_PUSH_IMM32: {
            out->opcode      = OPCODE_PUSH_IMM32;
            out->operand_type= OPERAND_IMM;
            out->imm         = *(uint32_t*)&code[offset]; offset+=4;
            out->rex         = rex;
            out->size        = offset;
            return offset;
        }
        case OPCODE_XCHG_REG_REG: {
            uint8_t modrm, mod, reg, rm;
            FETCH_MODRM();
            APPLY_REX_BITS();
            out->opcode      = OPCODE_XCHG_REG_REG;
            out->operand_type= OPERAND_REG|OPERAND_REG;
            out->op1         = rm;
            out->op2         = reg;
            out->rex         = rex;
            out->size        = offset;
            return offset;
        }
        case OPCODE_XOR_REG_IMM32: {
            uint8_t modrm, mod, reg, rm;
            FETCH_MODRM(); 
            APPLY_REX_BITS();
            if (((modrm >> 3)&7) == 6) {
                out->opcode      = OPCODE_XOR_REG_IMM32;
                out->operand_type= OPERAND_REG|OPERAND_IMM;
                out->op1         = rm;
                out->imm         = *(uint32_t*)&code[offset]; offset+=4;
                out->rex         = rex;
                out->size        = offset;
                return offset;
            }
            break;
        }
        case OPCODE_ADD_REG_REG: {
            uint8_t modrm, mod, reg, rm;
            FETCH_MODRM(); 
            APPLY_REX_BITS();
            out->opcode      = OPCODE_ADD_REG_REG;
            out->operand_type= OPERAND_REG|OPERAND_REG;
            out->op1         = rm;
            out->op2         = reg;
            out->rex         = rex;
            out->size        = offset;
            return offset;
        }
        case OPCODE_SUB_REG_REG: {
            uint8_t modrm, mod, reg, rm;
            FETCH_MODRM();
            APPLY_REX_BITS();
            out->opcode      = OPCODE_SUB_REG_REG;
            out->operand_type= OPERAND_REG|OPERAND_REG;
            out->op1         = rm;
            out->op2         = reg;
            out->rex         = rex;
            out->size        = offset;
            return offset;
        }
        case OPCODE_TEST_REG8_REG8: {
            uint8_t modrm, mod, reg, rm;
            FETCH_MODRM();
            APPLY_REX_BITS();
            out->opcode      = OPCODE_TEST_REG8_REG8;
            out->operand_type= OPERAND_REG|OPERAND_REG;
            out->op1         = rm;
            out->op2         = reg;
            out->rex         = rex;
            out->size        = offset;
            return offset;
        }
        case OPCODE_JCC_NEAR: {
            uint8_t ext = code[offset++];
            if (ext >= 0x80 && ext <= 0x8F) {
                out->opcode      = (opcode<<8)|ext;
                out->operand_type= OPERAND_IMM;
                out->imm         = *(int32_t*)&code[offset]; offset+=4;
                out->rex         = rex;
                out->size        = offset;
                return offset;
            }
            break;
        }
        case OPCODE_JMP_REL32: {
            out->opcode      = OPCODE_JMP_REL32;
            out->operand_type= OPERAND_IMM;
            out->imm         = *(int32_t*)&code[offset]; offset+=4;
            out->rex         = rex;
            out->size        = offset;
            return offset;
        }
        case OPCODE_JMP_REL8: {
            out->opcode      = OPCODE_JMP_REL8;
            out->operand_type= OPERAND_IMM;
            out->imm         = (int8_t)code[offset++];
            out->rex         = rex;
            out->size        = offset;
            return offset;
        }
        case OPCODE_CALL_R64: {
            uint8_t modrm, mod, reg, rm;
            FETCH_MODRM();
            APPLY_REX_BITS();
            if (((modrm>>3)&7)==2) {
                out->opcode      = OPCODE_CALL_R64;
                out->operand_type= OPERAND_REG;
                out->op1         = rm;
                out->rex         = rex;
                out->size        = offset;
                return offset;
            }
            break;
        }
    }
    return -1;
}