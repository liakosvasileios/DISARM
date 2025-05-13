#include "mba.h"


void xor_decomposition(struct Instruction *out, int target_reg, int temp_reg, uint32_t imm) {
    uint32_t mask = rand();     // Random 64-bit value
    uint32_t encoded = imm ^ mask;

    memset(out, 0, sizeof(struct Instruction) * 3);

    // mov temp_reg, encoded 
    out[0].opcode = 0xB8;       // mov reg, imm
    out[0].operand_type = OPERAND_REG | OPERAND_IMM;
    out[0].op1 = temp_reg;
    out[0].imm = encoded;
    out[0].rex = 0x48;

    // xor temp_reg, imm32
    out[1].opcode = 0x81;                            // xor r/m64, imm32
    out[1].operand_type = OPERAND_REG | OPERAND_IMM; // register + immediate
    out[1].op1 = temp_reg;
    out[1].imm = mask;
    out[1].rex = 0x48;
}

