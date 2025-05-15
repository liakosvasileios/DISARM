#ifndef ISA_H
#define ISA_H

#include <stdint.h>

// ====================================================================================================
// Instruction
// ====================================================================================================

struct Instruction{
    uint16_t opcode;             // Unique internal representation of the instrution opcode
    uint8_t operand_type;       // Operand types byte
    uint8_t op1;                // First operand (could be register ID, memory base reg, etc.)
    uint8_t op2;                // Second operand (same format as the first operand)
    uint32_t imm;               // Immediate or displacement value
    uint8_t size;               // Total size in bytes when reassembled
    uint8_t rex;                // REX prefix byte (0x40-0x4F for x86_64)

};

// ====================================================================================================
// Operand Types
// ====================================================================================================

enum OperandType {
    OPERAND_NONE    = 0x00,     // No operand
    OPERAND_REG     = 0x01,     // Register
    OPERAND_IMM     = 0x02,     // Immediate
    OPERAND_MEM     = 0x04,     // Memory access
    OPERAND_RIPREL  = 0x08,     // RIP-relative addressing
    OPERAND_DIPL    = 0x10,     // Displacement-based memory
    OPERAND_SIB     = 0x20,     // SIB-byte based memory
    OPERAND_PTR     = 0x40,     // Far pointer operand (not common, for completeness)
};

// ====================================================================================================
// Registers
// ====================================================================================================

enum Register64 {
    // 64-bit general purpose registers
    RAX_REG, RBX_REG, RCX_REG, RDX_REG,
    RSI_REG, RDI_REG, RBP_REG, RSP_REG,
    R8_REG, R9_REG, R10_REG, R11_REG,
    R12_REG, R13_REG, R14_REG, R15_REG,

    // Special pseudo-register
    REG_NONE = 0xFF     // No register sentinel
};

enum Register32 {
    // 32-bit general purpose registers
    EAX_REG, EBX_REG, ECX_REG, EDX_REG,
    ESI_REG, EDI_REG, EBP_REG, ESP_REG,
    REG_R8D, REG_R9D, REG_R10D, REG_R11D,
    REG_R12D, REG_R13D, REG_R14D, REG_R15D  
};

enum Register16 {
    // 16-bit general purpose registers
    AX_REG, BX_REG, CX_REG, DX_REG,
    SI_REG, DI_REG, BP_REG, SP_REG,
    R8W_REG, R9W_REG, R10W_REG, R11W_REG,
    R12W_REG, R13W_REG, R14W_REG, R15W
};

enum Register8 {
     // 8-bit general purpose registers
     AL_REG, BL_REG, CL_REG, DL_REG,
     SIL_REG, DIL_REG, BPL_REG, SPL_REG,
     R8B_REG, R9B_REG, R10B_REG, R11B_REG,
     R12B_REG, R13B_REG, R14B_REG, R15B_REG
};

// ====================================================================================================
// Opcodes
// ====================================================================================================

// MOV
#define OPCODE_MOV_REG_IMM64  0xB8  // B8+rd    
#define OPCODE_MOV_MEM_REG    0x89  // 89 /r
#define OPCODE_MOV_REG_MEM    0x8B  // 8B /r
#define OPCODE_MOV_MEM_IMM32  0xC7  // C7 04 24

// ARITHMETIC
#define OPCODE_ADD_RAX_IMM32  0x05  // 05 id
#define OPCODE_SUB_RAX_IMM32  0x2D  // 2D id
#define OPCODE_ADD_REG_REG    0x01  // 01 /r
#define OPCODE_SUB_REG_REG    0x29  // 29 /r

// LOGIC
#define OPCODE_XOR_REG_REG    0x31  // 31 /r
#define OPCODE_XOR_REG_IMM32  0x81  // 81 /6 id

// PUSH/POP
#define OPCODE_PUSH_IMM32     0x68  // 68 id

// EXCHANGE
#define OPCODE_XCHG_REG_REG   0x87  // 87 /r

// TEST
#define OPCODE_TEST_REG8_REG8 0x84  // 84 /r

// CONDITIONAL JUMPS
#define OPCODE_JCC_SHORT_MIN  0x70  // 70..7F
#define OPCODE_JCC_SHORT_MAX  0x7F
#define OPCODE_JCC_NEAR       0x0F  // 0F 8x

// SETcc
#define OPCODE_SETCC_BASE     0x90  // 0F 90..9F

// UNCONDITIONAL JUMP
#define OPCODE_JMP_REL32      0xE9
#define OPCODE_JMP_REL8       0xEB

#endif // ISA_H