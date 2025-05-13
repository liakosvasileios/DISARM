#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "engine.h"
#include "mutate.h"
#include "instruction.h"

#define MAX_INST_SIZE 15
#define MAX_MUTATED 4

void print_instruction(const char *label, const struct Instruction *inst) {
    printf("%s:\n", label);
    printf("  Opcode:        0x%02X\n", inst->opcode);
    printf("  Operand Type:  0x%02X\n", inst->operand_type);
    printf("  op1:           %d\n", inst->op1);
    printf("  op2:           %d\n", inst->op2);
    printf("  Immediate:     %d (0x%08X)\n", inst->imm, inst->imm);
    printf("  REX Prefix:    0x%02X\n", inst->rex);
    printf("\n");
}

/*
    - Loads an input binary file
    - Decodes instruction-by-instruction
    - Applies supported multi-instruction mutations using mutate_multi()
    - Falls back to mutate_opcode() if no multi-instruction match
    - Pads any leftover instruction bytes with NOPs
    - Writes a transformed .bin file
*/

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <input.bin> <output.bin> <0=deterministic, 1=random>\n", argv[0]);
        return 1;
    }

    int randomize = atoi(argv[3]);
    if (randomize) {
        srand(time(NULL));
    } else {
        srand(1); // deterministic seed for testing
    }

    FILE *in = fopen(argv[1], "rb");
    if (!in) {
        perror("fopen input");
        return 1;
    }

    fseek(in, 0, SEEK_END);
    long in_size = ftell(in);
    fseek(in, 0, SEEK_SET);

    uint8_t *buffer = malloc(in_size);
    if (!buffer) {
        perror("malloc");
        fclose(in);
        return 1;
    }
    fread(buffer, 1, in_size, in);
    fclose(in);

    FILE *out = fopen(argv[2], "wb");
    if (!out) {
        perror("fopen output");
        free(buffer);
        return 1;
    }

    size_t offset = 0;
    while (offset < in_size) {
        struct Instruction inst;
        int decoded = decode_instruction(&buffer[offset], &inst);

        if (decoded <= 0) {
            // Unrecognized byte, copy as-is
            fputc(buffer[offset], out);
            offset++;
            continue;
        }

        struct Instruction mutated[MAX_MUTATED];
        int count = mutate_multi(&inst, mutated, MAX_MUTATED);

        int total_written = 0;

        if (count > 0) {
            for (int i = 0; i < count; i++) {
                uint8_t encoded[MAX_INST_SIZE] = {0};
                int len = encode_instruction(&mutated[i], encoded);
                if (len > 0) {
                    fwrite(encoded, 1, len, out);
                    total_written += len;
                }
            }

            int pad = decoded - total_written;
            for (int i = 0; i < pad; i++) {
                fputc(0x90, out); // NOP
            }

            offset += decoded;
            continue; // SKIP mutate_opcode and original write
        } else {
            struct Instruction backup = inst;
            mutate_opcode(&inst);
            int changed = memcmp(&inst, &backup, sizeof(struct Instruction)) != 0;

            uint8_t encoded[MAX_INST_SIZE] = {0};
            int len = encode_instruction(changed ? &inst : &backup, encoded);
            if (len > 0) {
                fwrite(encoded, 1, len, out);
                printf("Instruction at offset 0x%04zx:\n", offset);
                printf("  len: %d\n", len);
                print_instruction("  Encoded", changed ? &inst : &backup);
                total_written = len;
            }
        }

        offset += decoded;
    }

    fclose(out);
    free(buffer);
    printf("[*] Transformation complete.\n");
    return 0;
}