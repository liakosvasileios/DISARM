#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "engine.h"
#include "operand_type.h"
#include "registers.h"

void print_instruction(const char *label, struct Instruction *inst) {
    printf("%s:\n", label);
    printf("  Opcode:        0x%X\n", inst->opcode);
    printf("  Operand Type:  0x%X\n", inst->operand_type);
    printf("  op1:           %u\n", inst->op1);
    printf("  op2:           %u\n", inst->op2);
    printf("  Immediate:     0x%X\n", inst->imm);
    printf("  REX Prefix:    0x%02X\n", inst->rex);
    printf("\n");
}

void test(const char *desc, uint8_t *bytes, size_t len, int verbose) {
    struct Instruction original, decoded;
    printf("[*] Testing: %s\n", desc);
    
    // Step 1: Decode
    if (verbose == 1) printf("[*] Decoding original instruction...\n");

    int decoded_size = decode_instruction(bytes, &original);
    assert(decoded_size > 0);

    if (verbose == 1) print_instruction("Before mutation", &original);

    // Optional mutation step
    // mutate_opcode(&original);
    // print_instruction("After mutation", &original);

    // Step 2: Encode
    uint8_t encoded[15] = {0};
    int encoded_size = encode_instruction(&original, encoded);
    assert(encoded_size > 0);

    if (verbose == 1) {
        printf("Encoded bytes: ");
        for (int i = 0; i < encoded_size; i++) {
            printf("%02X ", encoded[i]);
        }
        printf("\n");
    }

    // Step 3: Decode the encoded bytes again
    int red_size = decode_instruction(encoded, &decoded);
    assert(red_size > 0);

    // Step 4: Compare fields
    assert(decoded_size == red_size);
    assert(decoded.opcode == original.opcode);
    assert(decoded.operand_type == original.operand_type);
    assert(decoded.rex == original.rex);
    assert(decoded.op1 == original.op1);
    assert(decoded.op2 == original.op2);
    assert(decoded.imm == original.imm);

    printf("[+] Round-trip test passed.\n\n");
}

int main() {
    // mov rax, 0x0
    uint8_t mov_rax_imm[] = {0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    test("mov rax, 0x0", mov_rax_imm, sizeof(mov_rax_imm), 0);

    // add rax, 0x0
    uint8_t add_rax_imm[] = {0x48, 0x05, 0x00, 0x00, 0x00, 0x00};
    test("add rax, 0x0", add_rax_imm, sizeof(add_rax_imm), 0);

    // mov [rbx], rax
    uint8_t mov_mem_reg[] = {0x48, 0x89, 0x03};
    test("mov [rbx], rax", mov_mem_reg, sizeof(mov_mem_reg), 0);

    // mov eax, [rbx]
    uint8_t mov_reg_mem[] = {0x8B, 0x03};
    test("mov eax, [rbx]", mov_reg_mem, sizeof(mov_reg_mem), 0);

    // sub rax, 0x5
    uint8_t sub_rax_imm[] = {0x48, 0x2D, 0x05, 0x00, 0x00, 0x00};
    test("sub rax, 0x5", sub_rax_imm, sizeof(sub_rax_imm), 0);

    // xor rcx, rcx
    uint8_t xor_rcx[] = {0x48, 0x31, 0xC9};
    test("xor rcx, rcx", xor_rcx, sizeof(xor_rcx), 0);

    // push imm32
    uint8_t push_imm[] = {0x68, 0x34, 0x12, 0x00, 0x00};
    test("push imm32", push_imm, sizeof(push_imm), 0);

    // xchg rax, rbx
    uint8_t xchg_rax_rbx[] = {0x48, 0x87, 0xD8};
    test("xchg rax, rbx", xchg_rax_rbx, sizeof(xchg_rax_rbx), 0);

    uint8_t xor_imm[] = { 0x48, 0x81, 0xF0, 0x78, 0x56, 0x34, 0x12 }; // xor rax, 0x12345678
    test("xor m/r64, imm32", xor_imm, sizeof(xor_imm), 0);

    uint8_t add_reg_reg[] = {0x48, 0x01, 0xC8};  // add [RAX <- RAX + RCX]
    test("add rax, rcx", add_reg_reg, sizeof(add_reg_reg), 0);

    uint8_t sub_reg_reg[] = {0x48, 0x29, 0xC8};  // sub [RAX <- RAX - RCX]
    test("sub rax, rcx", sub_reg_reg, sizeof(sub_reg_reg), 0);

    uint8_t jcc_short[] = {0x74, 0x02};  // JE short +2
    test("JE short +2", jcc_short, sizeof(jcc_short), 0);

    uint8_t jcc_near[] = {0x0F, 0x85, 0x78, 0x56, 0x34, 0x12};  // JNE +0x12345678
    test("JNE near +0x12345678", jcc_near, sizeof(jcc_near), 0);

    uint8_t jnz_short[] = {0x75, 0x05};  // JNZ +5
    test("JNZ short +5", jnz_short, sizeof(jnz_short), 0);

    uint8_t jl_near[] = {0x0F, 0x8C, 0x10, 0x00, 0x00, 0x00};  // JL +0x10
    test("JL near +0x10", jl_near, sizeof(jl_near), 0);

    uint8_t test_al_al[] = {0x84, 0xC0};  // TEST AL, AL
    test("test al, al", test_al_al, sizeof(test_al_al), 1);

    return 0;
}
