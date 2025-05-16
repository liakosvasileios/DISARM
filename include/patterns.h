#ifndef PATTERNS_H
#define PATTERNS_H

#include "isa.h"

#define IS_MOV_REG_0(inst) ((inst)->opcode == OPCODE_MOV_REG_IMM64 && (inst)->imm == 0)

#define IS_MOV_REG_IMM32(inst) ((inst)->opcode == OPCODE_MOV_REG_IMM64 && (inst)->operand_type == (OPERAND_REG | OPERAND_IMM))

#define IS_MOV_RM_REG(inst) ((inst)->opcode == OPCODE_MOV_MEM_REG && (inst)->operand_type == (OPERAND_MEM | OPERAND_REG))

#define IS_XOR_REG_REG(inst) ((inst)->opcode == OPCODE_XOR_REG_REG && (inst)->operand_type == (OPERAND_REG | OPERAND_REG))

#define IS_PUSH_IMM32(inst) ((inst)->opcode == OPCODE_PUSH_IMM32 && (inst)->operand_type == OPERAND_IMM) 

#define IS_XCHG_REG_REG(inst) ((inst)->opcode == OPCODE_XCHG_REG_REG && (inst)->operand_type == (OPERAND_REG | OPERAND_REG))

#define IS_ADD_RAX_IMM32(inst) ((inst)->opcode == OPCODE_ADD_RAX_IMM32 && (inst)->operand_type == (OPERAND_REG | OPERAND_IMM) && (inst)->op1 == RAX_REG)

#define IS_SUB_RAX_IMM32(inst) ((inst)->opcode == OPCODE_SUB_RAX_IMM32 && (inst)->operand_type == (OPERAND_REG | OPERAND_IMM) && (inst)->op1 == RAX_REG)

#define IS_JCC_NEAR(inst) ((((inst)->opcode & 0xFF00) == 0x0F00) && ((inst)->operand_type == OPERAND_IMM))

#define IS_CALL_REL32(inst) ((inst)->opcode == OPCODE_CALL_REL32 && (inst)->operand_type == OPERAND_IMM)

#endif // PATTERNS_H