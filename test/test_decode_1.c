#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "engine.h"
#include "operand_type.h"
#include "registers.h"


void test_mov_reg_imm() {
    struct Instruction original = {
        .opcode = 0xB8,
        .operand_type = OPERAND_REG | OPERAND_IMM,
        .op1 = RAX_REG,
        .op2 = 0,
        .imm = 0x12345678,  
        .size = 0
    };

    uint8_t buffer[16];
    int encoded_size = encode_instruction(&original, buffer);
    assert(encoded_size > 0);

    struct Instruction decoded;
    int decoded_size = decode_instruction(buffer, &decoded);

    assert(decoded_size == encoded_size);
    assert(decoded.opcode == original.opcode);
    assert(decoded.operand_type == original.operand_type);
    assert(decoded.op1 == original.op1);
    assert(decoded.imm == original.imm); 

    printf("[✓] test_mov_reg_imm passed\n");
}

void test_add_rax_imm() {
    struct Instruction original = {
        .opcode = 0x05,
        .operand_type = OPERAND_REG | OPERAND_IMM,
        .op1 = RAX_REG,
        .op2 = 0,
        .imm = 0x90ABCDEF,  
        .size = 0
    };

    uint8_t buffer[16];
    int encoded_size = encode_instruction(&original, buffer);
    assert(encoded_size > 0);

    struct Instruction decoded;
    int decoded_size = decode_instruction(buffer, &decoded);

    assert(decoded_size == encoded_size);
    assert(decoded.opcode == original.opcode);
    assert(decoded.operand_type == original.operand_type);
    assert(decoded.op1 == original.op1);
    assert(decoded.imm == original.imm); 

    printf("[✓] test_add_rax_imm passed\n");
}

int main() {
    test_mov_reg_imm();
    test_add_rax_imm();

    // Add more test cases as needed.
    printf("\nAll encoding/decoding tests passed.\n");
    return 0;
}
