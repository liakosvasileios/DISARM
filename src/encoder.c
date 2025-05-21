#include <string.h>
#include <stdio.h>
#include "engine.h"
#include "isa.h"

// helper to emit SIB+disp
static int emit_sib_disp(const struct Instruction* inst, uint8_t* out) {
    int off = 0;

    uint8_t sib = ((inst->scale & 3) << 6)
                | ((inst->index == REG_INVALID ? 4 : (inst->index & 7)) << 3)
                | (inst->base & 7);
    out[off++] = sib;

    if (inst->disp != 0) {
        if (inst->disp >= -128 && inst->disp <= 127) {
            out[off++] = (int8_t)inst->disp;
        } else {
            memcpy(&out[off], &inst->disp, 4);
            off += 4;
        }
    }

    return off;
}

int encode_instruction(const struct Instruction* inst, uint8_t* out) {
    int offset = 0; 
    uint8_t mod;
    uint8_t rex = inst->rex;

    // emit REX
    if (rex) {
        EMIT(rex);
    } else if (inst->op1 >= 8 || inst->op2 >= 8) {
        rex = 0x48;
        if (inst->op1 >= 8) rex |= 0x01;
        if (inst->op2 >= 8) rex |= 0x04;
        EMIT(rex);
    }

    // Short jumps
    if (inst->opcode >= 0x70 && inst->opcode <= 0x7F) {
        EMIT(inst->opcode);                    // Emit Jcc short opcode
        EMIT((int8_t)inst->imm);               // Emit signed 8-bit offset
        return offset;
    }

    // Jcc near
    if ((inst->opcode & 0xFF00) == 0x0F00 && (inst->opcode & 0xFF) >= 0x80 && (inst->opcode & 0xFF) <= 0x8F) {
        EMIT(0x0F);
        EMIT(inst->opcode & 0xFF);
        memcpy(&out[offset], &inst->imm, 4); offset += 4;
        return offset;
    }

    if ((inst->opcode & 0xFF00) == 0x0F00 && (inst->opcode & 0xFF) >= 0x90 && (inst->opcode & 0xFF) <= 0x9F) {
        EMIT(0x0F);
        EMIT(inst->opcode & 0xFF);
        EMIT(ENCODE_MODRM(3, 0, inst->op1)); // reg = condition (ignored), rm = target reg
        return offset;
    }

    switch (inst->opcode) {
        case OPCODE_MOV_REG_IMM64: {
            uint8_t hw = inst->op1 & 0x07;
            EMIT(0xB8 + hw);
            if (rex & 0x08) {
                memcpy(&out[offset], &inst->imm, 8); offset += 8;
            } else {
                memcpy(&out[offset], &inst->imm, 4); offset += 4;
            }
            return offset;
        }

        case OPCODE_MOV_MEM_REG: {
            // [mem] ← reg
            if (inst->disp == 0 && inst->base != REG_INVALID && inst->base != 5) {
                mod = 0;
            } else if (inst->disp >= -128 && inst->disp <= 127) {
                mod = 1;
            } else {
                mod = 2;
            }

            int needs_sib = (inst->index != REG_INVALID ||
                              inst->base == RSP_REG || inst->base == R12_REG);

            EMIT(OPCODE_MOV_MEM_REG);

            if (needs_sib) {
                EMIT((mod << 6) | ((inst->op2 & 7) << 3) | 4);  // rm=4 = SIB follows
                offset += emit_sib_disp(inst, &out[offset]);
            } else {
                EMIT((mod << 6) | ((inst->op2 & 7) << 3) | (inst->base & 7));
                if (mod == 1) {
                    EMIT((int8_t)inst->disp);
                } else if (mod == 2 || (mod == 0 && inst->base == REG_INVALID)) {
                    memcpy(&out[offset], &inst->disp, 4); offset += 4;
                }
            }

            return offset;
        }

        case OPCODE_MOV_REG_MEM: {
            // reg ← [mem]
            if (inst->disp == 0 && inst->base != REG_INVALID && inst->base != 5) {
                mod = 0;
            } else if (inst->disp >= -128 && inst->disp <= 127) {
                mod = 1;
            } else {
                mod = 2;
            }

            int needs_sib = (inst->index != REG_INVALID ||
                              inst->base == RSP_REG || inst->base == R12_REG);

            EMIT(OPCODE_MOV_REG_MEM);

            if (needs_sib) {
                EMIT((mod << 6) | ((inst->op1 & 7) << 3) | 4);
                offset += emit_sib_disp(inst, &out[offset]);
            } else {
                EMIT((mod << 6) | ((inst->op1 & 7) << 3) | (inst->base & 7));
                if (mod == 1) {
                    EMIT((int8_t)inst->disp);
                } else if (mod == 2 || (mod == 0 && inst->base == REG_INVALID)) {
                    memcpy(&out[offset], &inst->disp, 4); offset += 4;
                }
            }

            return offset;
        }

        case OPCODE_ADD_RAX_IMM32: {
            EMIT(OPCODE_ADD_RAX_IMM32);
            memcpy(&out[offset], &inst->imm, 4); offset += 4;
            return offset;
        }

        case OPCODE_SUB_RAX_IMM32: {
            EMIT(OPCODE_SUB_RAX_IMM32);
            memcpy(&out[offset], &inst->imm, 4); offset += 4;
            return offset;
        }

        case OPCODE_XOR_REG_REG: {
            EMIT(OPCODE_XOR_REG_REG);
            EMIT(ENCODE_MODRM(3, inst->op1, inst->op2));
            return offset;
        }

        case OPCODE_PUSH_IMM32: {
            EMIT(OPCODE_PUSH_IMM32);
            memcpy(&out[offset], &inst->imm, 4); offset += 4;
            return offset;
        }

        case OPCODE_XCHG_REG_REG: {
            EMIT(OPCODE_XCHG_REG_REG);
            EMIT(ENCODE_MODRM(3, inst->op1, inst->op2));
            return offset;
        }

        case OPCODE_XOR_REG_IMM32: {
            EMIT(OPCODE_XOR_REG_IMM32);
            EMIT((inst->op1 & 7) | 0x30 | ((6 & 7) << 3));
            memcpy(&out[offset], &inst->imm, 4); offset += 4;
            return offset;
        }

        case OPCODE_ADD_REG_REG: {
            EMIT(OPCODE_ADD_REG_REG);
            EMIT(ENCODE_MODRM(3, inst->op1, inst->op2));
            return offset;
        }

        case OPCODE_SUB_REG_REG: {
            EMIT(OPCODE_SUB_REG_REG);
            EMIT(ENCODE_MODRM(3, inst->op1, inst->op2));
            return offset;
        }

        case OPCODE_TEST_REG8_REG8: {
            EMIT(OPCODE_TEST_REG8_REG8);
            EMIT(ENCODE_MODRM(3, inst->op1, inst->op2));
            return offset;
        }

        case OPCODE_CALL_REL32: {
            EMIT(OPCODE_CALL_REL32);
            memcpy(&out[offset], &inst->imm, 4); offset += 4;
            return offset;
        }

        case OPCODE_CALL_R64: {
            EMIT(OPCODE_CALL_R64);
            EMIT(0xD0 | (inst->op1 & 7));
            return offset;
        }

        // case OPCODE_JCC_NEAR: {
        //     EMIT(0x0F);
        //     EMIT(inst->opcode & 0xFF);
        //     memcpy(&out[offset], &inst->imm, 4); offset += 4;
        //     return offset;
        // }

        case OPCODE_JMP_REL32: {
            EMIT(OPCODE_JMP_REL32);
            memcpy(&out[offset], &inst->imm, 4); offset += 4;
            return offset;
        }

        case OPCODE_JMP_REL8: {
            EMIT(OPCODE_JMP_REL8);
            EMIT((int8_t)inst->imm);
            return offset;
        }
    }

    return -1;
}
