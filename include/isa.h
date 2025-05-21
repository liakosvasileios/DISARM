#ifndef ISA_H
#define ISA_H

#include <stdint.h>

// ====================================================================================================
// Instruction
// ====================================================================================================

struct Instruction {
    uint16_t opcode;        // internal opcode
    uint8_t  operand_type;  // operand types
    uint8_t  op1, op2;      // registers or mem specifier
    uint64_t imm;           // immediate
    int32_t  disp;          // displacement for memory
    uint8_t  scale;         // SIB scale
    uint8_t  index;         // SIB index
    uint8_t  base;          // SIB base
    uint8_t  rex;           // REX prefix
    uint8_t  size;          // total size
};

// Operand Types
enum OperandType {
    OPERAND_NONE   = 0x00,
    OPERAND_REG    = 0x01,
    OPERAND_IMM    = 0x02,
    OPERAND_MEM    = 0x04,
    OPERAND_RIPREL = 0x08,
    OPERAND_DIPL   = 0x10,
    OPERAND_SIB    = 0x20,
    OPERAND_PTR    = 0x40,
};

// Registers now match hardware encodings
enum Register64 {
    RAX_REG=0, RCX_REG=1, RDX_REG=2, RBX_REG=3,
    RSP_REG=4, RBP_REG=5, RSI_REG=6, RDI_REG=7,
    R8_REG=8,  R9_REG=9,  R10_REG=10, R11_REG=11,
    R12_REG=12, R13_REG=13, R14_REG=14, R15_REG=15,
    REG_INVALID=0xFF
};

enum Register32 {
    EAX_REG=0, ECX_REG=1, EDX_REG=2, EBX_REG=3,
    ESP_REG=4, EBP_REG=5, ESI_REG=6, EDI_REG=7,
    REG_R8D=8, REG_R9D=9, REG_R10D=10, REG_R11D=11,
    REG_R12D=12, REG_R13D=13, REG_R14D=14, REG_R15D=15
};

enum Register16 {
    AX_REG=0,  CX_REG=1, DX_REG=2, BX_REG=3,
    SP_REG=4,  BP_REG=5, SI_REG=6, DI_REG=7,
    R8W_REG=8, R9W_REG=9, R10W_REG=10, R11W_REG=11,
    R12W_REG=12, R13W_REG=13, R14W_REG=14, R15W_REG=15
};

enum Register8 {
    AL_REG=0,  CL_REG=1, DL_REG=2,  BL_REG=3,
    SPL_REG=4, BPL_REG=5, SIL_REG=6, DIL_REG=7,
    R8B_REG=8, R9B_REG=9, R10B_REG=10, R11B_REG=11,
    R12B_REG=12, R13B_REG=13, R14B_REG=14, R15B_REG=15
};

// Opcodes
#define OPCODE_MOV_REG_IMM64    0xB8 // B8+rd
#define OPCODE_MOV_MEM_REG      0x89 // 89 /r
#define OPCODE_MOV_REG_MEM      0x8B // 8B /r
#define OPCODE_MOV_MEM_IMM32    0xC7 // C7 04 24 + imm32
#define OPCODE_ADD_RAX_IMM32    0x05 // 05 id
#define OPCODE_SUB_RAX_IMM32    0x2D // 2D id
#define OPCODE_ADD_REG_REG      0x01 // 01 /r
#define OPCODE_SUB_REG_REG      0x29 // 29 /r
#define OPCODE_XOR_REG_REG      0x31 // 31 /r
#define OPCODE_XOR_REG_IMM32    0x81 // 81 /6 id
#define OPCODE_PUSH_IMM32       0x68 // 68 id
#define OPCODE_XCHG_REG_REG     0x87 // 87 /r
#define OPCODE_TEST_REG8_REG8   0x84 // 84 /r
#define OPCODE_JCC_SHORT_MIN    0x70 // 70..7F
#define OPCODE_JCC_SHORT_MAX    0x7F
#define OPCODE_JCC_NEAR         0x0F // 0F 8x
#define OPCODE_JMP_REL32        0xE9
#define OPCODE_JMP_REL8         0xEB
#define OPCODE_CALL_REL32       0xE8 // E8 id
#define OPCODE_CALL_R64         0xFF // FF /2

#endif // ISA_H