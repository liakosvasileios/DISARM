#ifndef PATTERNS_H
#define PATTERNS_H

#include "isa.h"

#define IS_MOV_REG_0(inst) ((inst)->opcode == 0xB8 && (inst)->imm == 0)

#define IS_MOV_REG_IMM32(inst) ((inst)->opcode == 0xB8 && (inst)->operand_type == (OPERAND_REG | OPERAND_IMM))

#define IS_MOV_RM_REG(inst) ((inst)->opcode == 0x89 && (inst)->operand_type == (OPERAND_MEM | OPERAND_REG))

#define IS_XOR_REG_REG(inst) ((inst)->opcode == 0x31 && (inst)->operand_type == (OPERAND_REG | OPERAND_REG))

#define IS_PUSH_IMM32(inst) ((inst)->opcode == 0x68 && (inst)->operand_type == OPERAND_IMM) 

#define IS_XCHG_REG_REG(inst) ((inst)->opcode == 0x87 && (inst)->operand_type == (OPERAND_REG | OPERAND_REG))

#define IS_ADD_RAX_IMM32(inst) ((inst)->opcode == 0x05 && (inst)->operand_type == (OPERAND_REG | OPERAND_IMM) && (inst)->op1 == RAX_REG)

#define IS_SUB_RAX_IMM32(inst) ((inst)->opcode == 0x2D && (inst)->operand_type == (OPERAND_REG | OPERAND_IMM) && (inst)->op1 == RAX_REG)

#define IS_JCC_NEAR(inst) (((inst)->opcode & 0xFF00) == 0x0F00 && (inst)->operand_type == OPERAND_IMM )

#endif // PATTERNS_H