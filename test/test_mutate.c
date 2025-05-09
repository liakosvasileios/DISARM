#include <stdio.h>
#include "engine.h"
#include "mutate.h"

void print_instruction(const char *label, struct Instruction *inst) {
    printf("%s:\n", label);
    printf("  Opcode:        0x%02X\n", inst->opcode);
    printf("  Operand Type:  0x%02X\n", inst->operand_type);
    printf("  op1:           %u\n", inst->op1);
    printf("  op2:           %u\n", inst->op2);
    printf("  Immediate:     0x%X\n", inst->imm);
    printf("  REX Prefix:    0x%02X\n\n", inst->rex);
}

void test_mutation(const char *desc, uint8_t *bytes, size_t len) {
    struct Instruction inst;
    printf("[*] Test: %s\n", desc);

    if (decode_instruction(bytes, &inst) < 0) {
        printf("Failed to decode instruction.\n\n");
        return;
    }

    print_instruction("Original", &inst);

    printf("Encoded original: ");
    uint8_t encodedb4[15] = {0};
    int enc_lenb4 = encode_instruction(&inst, encodedb4);
    for (int i = 0; i < enc_lenb4; i++) {
        printf("%02X ", encodedb4[i]);
    }
    printf("\n");

    // Try multi-instruction mutation
    struct Instruction multi[4];
    int count = mutate_multi(&inst, multi, 4);

    if (count > 0) {
        printf("[*] Multi-instruction mutation (%d instructions):\n", count);
        for (int i = 0; i < count; i++) {
            print_instruction("Mutated", &multi[i]);

            uint8_t enc[15] = {0};
            int enc_len = encode_instruction(&multi[i], enc);
            printf("Encoded: ");
            for (int j = 0; j < enc_len; j++) {
                printf("%02X ", enc[j]);
            }
            printf("\n");
        }
    } else {
        // Fall back to single-instruction mutation
        mutate_opcode(&inst);
        print_instruction("After mutate_opcode", &inst);

        uint8_t encoded[15] = {0};
        int enc_len = encode_instruction(&inst, encoded);
        if (enc_len <= 0) {
            printf("Encoding failed\n\n");
            return;
        }

        printf("Encoded single mutation: ");
        for (int i = 0; i < enc_len; i++) {
            printf("%02X ", encoded[i]);
        }
        printf("\n");
    }

    printf("\n");
}


int main() {
    uint8_t mov_reg_imm[] = {0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // mov rax, 0x0
    uint8_t add_reg_imm[] = {0x48, 0x05, 0x34, 0x12, 0x00, 0x00}; // add rax, 0x1234
    uint8_t mov_reg_reg[] = {0x48, 0x89, 0xD8}; // mov rax, rbx
    uint8_t sub_reg_imm[] = {0x48, 0x2D, 0x05, 0x00, 0x00, 0x00};    // sub rax, 0x5
    uint8_t xor_reg_self[] = {0x48, 0x31, 0xC0};    // xor rax, rax
    uint8_t push_imm[] = {0x68, 0x34, 0x12, 0x00, 0x00};    // push 0x1234
    uint8_t xchg_reg[] = {0x48, 0x87, 0xD8};    // xchg rax, rbx

    test_mutation("mov rax, 0x0 => xor rax, rax", mov_reg_imm, sizeof(mov_reg_imm));
    test_mutation("add rax, 0x1234 => sub rax, -0x1234", add_reg_imm, sizeof(add_reg_imm));
    test_mutation("mov [rax], rbx => mov rbx, [rax]", mov_reg_reg, sizeof(mov_reg_reg));
    test_mutation("sub rax, 0x5 => add rax, -0x5", sub_reg_imm, sizeof(sub_reg_imm));
    test_mutation("xor rax, rax => mov rax, 0x0", xor_reg_self, sizeof(xor_reg_self));
    test_mutation("push 0x1234 => sub rsp, 8 || mov [rsp], 0x1234", push_imm, sizeof(push_imm));
    test_mutation("xchg rax, rbx => xor rax, rbx; xor rbx, rax; xor rax, rbx", xchg_reg, sizeof(xchg_reg));

    return 0;
}
