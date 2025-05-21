#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "engine.h"
#include "isa.h"

void print_instruction(const char *label, struct Instruction *inst) {
    printf("%s:\n", label);
    printf("  Opcode:       0x%X\n", inst->opcode);
    printf("  Operands:     0x%X\n", inst->operand_type);
    printf("  op1,op2:      %u, %u\n", inst->op1, inst->op2);
    printf("  Imm:          0x%llX\n", (unsigned long long)inst->imm);
    printf("  Disp:         %d\n", inst->disp);
    printf("  SIB: scale=%u, idx=%u, base=%u\n", inst->scale, inst->index, inst->base);
    printf("  REX:          0x%02X\n", inst->rex);
    printf("  Size:         %u bytes\n", inst->size);
    printf("\n");
}

void print_bytes(const char *label, const uint8_t *bytes, size_t len) {
    printf("%s (%zu bytes):\n", label, len);
    for (size_t i = 0; i < len; i++) {
        printf("  %02X", bytes[i]);
    }
    printf("\n\n");
}

void test(const char *desc, const uint8_t *bytes, size_t len, int verbose) {
    struct Instruction orig, dec;

    printf("[] Testing %s\n", desc);

    int s1 = decode_instruction(bytes, &orig);

    if (verbose) {
        print_instruction("Original decoded", &orig);
        print_bytes("Original bytes", bytes, len);
    }

    assert(s1 > 0);

    uint8_t enc[64]; 
    memset(enc, 0, sizeof(enc));
    int s2 = encode_instruction(&orig, enc);

    if (verbose) {
        print_instruction("Encoded", &orig);
        print_bytes("Encoded bytes", enc, s2);
    }

    assert(s2 > 0);

    int s3 = decode_instruction(enc, &dec);

    if (verbose) {
        print_instruction("Encoded-Decoded", &dec);
        print_bytes("Encoded-Decoded bytes", enc, s3);
    }

    assert(s3 > 0);

    assert(s1 == s3);
    assert(orig.opcode == dec.opcode);
    assert(orig.operand_type == dec.operand_type);
    assert(orig.op1 == dec.op1 && orig.op2 == dec.op2);
    assert(orig.imm == dec.imm);
    assert(orig.disp == dec.disp);
    assert(orig.scale == dec.scale);
    assert(orig.index == dec.index);
    assert(orig.base == dec.base);

    printf("[+] %s passed.\n\n", desc);
}

int main() {
    // Manual encode/decode test for mov ecx, 3 (32-bit)
    // struct Instruction original = {
    //     .opcode = 0xB8,
    //     .operand_type = OPERAND_REG | OPERAND_IMM,
    //     .op1 = ECX_REG, // 1
    //     .imm = 3,
    //     .rex = 0x00
    // };

    // uint8_t encoded[16] = {0};
    // int len = encode_instruction(&original, encoded);

    // if (len <= 0) {
    //     printf("Encoding failed!\n");
    //     return 1;
    // }

    // printf("[*] Encoded Bytes (%d bytes):\n", len);
    // for (int i = 0; i < len; i++) {
    //     printf("  %02X", encoded[i]);
    // }
    // printf("\n\n");

    // struct Instruction decoded;
    // int decoded_len = decode_instruction(encoded, &decoded);
    // if (decoded_len <= 0) {
    //     printf("Decoding failed!\n");
    //     return 1;
    // }

    // print_instruction("[Original]", &original);
    // print_instruction("[Decoded ]", &decoded);

    // --- predefined tests ---
    uint8_t mov_rax_imm[] = {0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    test("mov rax, 0x0", mov_rax_imm, sizeof(mov_rax_imm), 0);

    uint8_t add_rax_imm[] = {0x48, 0x05, 0x00, 0x00, 0x00, 0x00};
    test("add rax, 0x0", add_rax_imm, sizeof(add_rax_imm), 0);

    uint8_t mov_mem_reg[] = {0x48, 0x89, 0x03};
    test("mov [rbx], rax", mov_mem_reg, sizeof(mov_mem_reg), 0);

    uint8_t mov_reg_mem[] = {0x8B, 0x03};
    test("mov eax, [rbx]", mov_reg_mem, sizeof(mov_reg_mem), 0);

    uint8_t sub_rax_imm[] = {0x48, 0x2D, 0x05, 0x00, 0x00, 0x00};
    test("sub rax, 0x5", sub_rax_imm, sizeof(sub_rax_imm), 0);

    uint8_t xor_rcx[] = {0x48, 0x31, 0xC9};
    test("xor rcx, rcx", xor_rcx, sizeof(xor_rcx), 0);

    uint8_t push_imm[] = {0x68, 0x34, 0x12, 0x00, 0x00};
    test("push imm32", push_imm, sizeof(push_imm), 0);

    uint8_t xchg_rax_rbx[] = {0x48, 0x87, 0xD8};
    test("xchg rax, rbx", xchg_rax_rbx, sizeof(xchg_rax_rbx), 0);

    uint8_t xor_imm[] = { 0x48, 0x81, 0xF0, 0x78, 0x56, 0x34, 0x12 };
    test("xor m/r64, imm32", xor_imm, sizeof(xor_imm), 0);

    uint8_t add_reg_reg[] = {0x48, 0x01, 0xC8};
    test("add rax, rcx", add_reg_reg, sizeof(add_reg_reg), 0);

    uint8_t sub_reg_reg[] = {0x48, 0x29, 0xC8};
    test("sub rax, rcx", sub_reg_reg, sizeof(sub_reg_reg), 0);

    uint8_t jcc_short[] = {0x74, 0x02};
    test("JE short +2", jcc_short, sizeof(jcc_short), 0);

    uint8_t jcc_near[] = {0x0F, 0x85, 0x78, 0x56, 0x34, 0x12};
    test("JNE near +0x12345678", jcc_near, sizeof(jcc_near), 0);

    uint8_t jnz_short[] = {0x75, 0x05};
    test("JNZ short +5", jnz_short, sizeof(jnz_short), 0);

    uint8_t jl_near[] = {0x0F, 0x8C, 0x10, 0x00, 0x00, 0x00};
    test("JL near +0x10", jl_near, sizeof(jl_near), 0);

    uint8_t test_al_al[] = {0x84, 0xC0};
    test("test al, al (test r/m8, r8)", test_al_al, sizeof(test_al_al), 0);

    uint8_t setne_al[] = {0x0F, 0x95, 0xC0};
    test("setne al", setne_al, sizeof(setne_al), 0);

    // SIB tests:
    uint8_t sib1[] = {0x48,0x8B,0x04,0xD3};            // mov rax,[rbx+rdx8]
    test("mov rax,[rbx+rdx8]", sib1, sizeof(sib1), 0);

    uint8_t sib2[] = {0x48,0x8B,0x44,0xD3,0x10};       // mov rax,[rbx+rdx8+0x10]
    test("mov rax,[rbx+rdx8+0x10]", sib2, sizeof(sib2), 0);

    return 0;
}
