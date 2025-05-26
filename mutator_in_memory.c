
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "engine.h"
#include "mutate.h"
#include "isa.h"

#define MAX_INST_SIZE 15
#define MAX_MUTATED 4

void xor_encrypt(uint8_t *data, size_t len, uint8_t key) {
    for (size_t i = 0; i < len; ++i)
        data[i] ^= key;
}

void print_instruction(const char *label, const struct Instruction *inst) {
    printf("%s:\n", label);
    printf("  Opcode:        0x%02X\n", inst->opcode);
    printf("  Operand Type:  0x%02X\n", inst->operand_type);
    printf("  op1:           %d\n", inst->op1);
    printf("  op2:           %d\n", inst->op2);
    printf("  Immediate:     %lld (0x%08llX)\n", inst->imm, inst->imm);
    printf("  REX Prefix:    0x%02X\n", inst->rex);
    printf("\n");
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input.bin> <0=deterministic, 1=random>\n", argv[0]);
        return 1;
    }

    int randomize = atoi(argv[2]);
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

    uint8_t *mutated_buf = malloc(in_size);
    if (!mutated_buf) {
        perror("malloc mutated_buf");
        free(buffer);
        return 1;
    }

    size_t offset = 0;
    size_t write_offset = 0;

    while (offset < in_size) {
        struct Instruction inst;
        int decoded = decode_instruction(&buffer[offset], &inst);

        if (decoded <= 0) {
            mutated_buf[write_offset++] = buffer[offset++];
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
                    memcpy(mutated_buf + write_offset, encoded, len);
                    write_offset += len;
                    total_written += len;
                }
            }
            int pad = decoded - total_written;
            memset(mutated_buf + write_offset, 0x90, pad);
            write_offset += pad;
            offset += decoded;
            continue;
        } else {
            struct Instruction backup = inst;
            mutate_opcode(&inst);
            int changed = memcmp(&inst, &backup, sizeof(struct Instruction)) != 0;

            uint8_t encoded[MAX_INST_SIZE] = {0};
            int len = encode_instruction(changed ? &inst : &backup, encoded);
            if (len > 0) {
                memcpy(mutated_buf + write_offset, encoded, len);
                write_offset += len;
                printf("Instruction at offset 0x%04zx:\n", offset);
                printf("  len: %d\n", len);
                print_instruction("  Encoded", changed ? &inst : &backup);
            }
        }

        offset += decoded;
    }

    xor_encrypt(mutated_buf, write_offset, 0x5A);

    printf("[*] Mutation complete. Mutated size: %zu bytes\n", write_offset);

    // Optional: save for debugging
    FILE *debug_out = fopen("debug_mutated_output.bin", "wb");
    if (debug_out) {
        fwrite(mutated_buf, 1, write_offset, debug_out);
        fclose(debug_out);
        printf("[*] Debug output written to debug_mutated_output.bin\n");
    }

    free(buffer);
    free(mutated_buf);
    return 0;
}
