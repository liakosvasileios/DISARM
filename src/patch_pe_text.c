#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "pe_structs.h"
#include "engine.h"
#include "mutate.h"
#include "isa.h"

#define MAX_INST_SIZE 15
#define MAX_MUTATED 4

int extract_text_section(const char *filename, uint8_t **output, size_t *size, uint32_t *entry_rva);

int verbose = 0;
FILE *log_file = NULL;

#define logf(...) do { \
    if (log_file) fprintf(log_file, __VA_ARGS__); \
    if (!verbose) printf(__VA_ARGS__); \
} while (0)

void print_instruction(const char *label, struct Instruction *inst) {
    logf("%s:\n", label);
    logf("  Opcode:       0x%X\n", inst->opcode);
    logf("  Operands:     0x%X\n", inst->operand_type);
    logf("  op1,op2:      %u, %u\n", inst->op1, inst->op2);
    logf("  Imm:          0x%llX\n", (unsigned long long)inst->imm);
    logf("  Disp:         %d\n", inst->disp);
    logf("  SIB: scale=%u, idx=%u, base=%u\n", inst->scale, inst->index, inst->base);
    logf("  REX:          0x%02X\n", inst->rex);
    logf("  Size:         %u bytes\n\n", inst->size);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <input_pe.exe> <output_pe.exe> <0=deterministic, 1=random> <1=file-only, 0=console+file>\n", argv[0]);
        return 1;
    }

    const char *input_pe = argv[1];
    const char *output_pe = argv[2];
    int randomize = atoi(argv[3]);
    verbose = atoi(argv[4]);

    log_file = fopen("patch_log.txt", "w");
    if (!log_file) {
        perror("[x] Could not open patch_log.txt");
        return 1;
    }

    logf("[*] Starting PE mutation process...\n");

    if (randomize) {
        logf("[*] Seeding RNG with current time...\n");
        srand(time(NULL));
    } else {
        logf("[*] Seeding RNG deterministically for reproducible mutations...\n");
        srand(1);
    }

    logf("[*] Extracting .text section from '%s'...\n", input_pe);
    uint8_t *text_data = NULL;
    size_t text_size = 0;
    uint32_t entry_rva = 0;
    if (extract_text_section(input_pe, &text_data, &text_size, &entry_rva) != 0) {
        fprintf(stderr, "[x] Failed to extract .text section\n");
        return 1;
    }

    logf("[+] .text extracted: %zu bytes, Entry Point RVA: 0x%X\n", text_size, entry_rva);

    logf("[*] Allocating buffer for mutated .text section...\n");
    uint8_t *mutated = malloc(text_size * 2); // Allow space for expansion
    if (!mutated) {
        perror("malloc mutated");
        free(text_data);
        return 1;
    }
    memset(mutated, 0x90, text_size * 2);

    logf("[*] Starting instruction-by-instruction transformation...\n");

    size_t offset = 0;
    size_t write_offset = 0;
    while (offset < text_size) {
        struct Instruction inst;
        int decoded = decode_instruction(&text_data[offset], &inst);

        if (decoded <= 0) {
            mutated[write_offset++] = text_data[offset++];
            continue;
        }

        struct Instruction mutated_list[MAX_MUTATED];
        int count = mutate_multi(&inst, mutated_list, MAX_MUTATED);
        int total_written = 0;

        if (count > 0) {
            for (int i = 0; i < count; i++) {
                uint8_t encoded[MAX_INST_SIZE] = {0};
                int len = encode_instruction(&mutated_list[i], encoded);
                if (len > 0) {
                    memcpy(&mutated[write_offset], encoded, len);
                    write_offset += len;
                    total_written += len;
                }
            }
        } else {
            struct Instruction backup = inst;
            mutate_opcode(&inst);
            int changed = memcmp(&inst, &backup, sizeof(struct Instruction)) != 0;

            uint8_t encoded[MAX_INST_SIZE] = {0};
            int len = encode_instruction(changed ? &inst : &backup, encoded);
            if (len > 0) {
                memcpy(&mutated[write_offset], encoded, len);
                write_offset += len;
                total_written = len;
            }
        }

        while (total_written < decoded) {
            mutated[write_offset++] = 0x90;
            total_written++;
        }

        offset += decoded;
    }

    logf("[+] Finished mutation. Total bytes written: %zu\n", write_offset);

    logf("[*] Reading original PE file...\n");

    FILE *f_in = fopen(input_pe, "rb");
    FILE *f_out = fopen(output_pe, "wb");
    if (!f_in || !f_out) {
        perror("[x] fopen");
        return 1;
    }

    fseek(f_in, 0, SEEK_END);
    long pe_size = ftell(f_in);
    rewind(f_in);

    uint8_t *pe_buf = malloc(pe_size + write_offset + 0x1000); // extra padding
    fread(pe_buf, 1, pe_size, f_in);
    fclose(f_in);

    logf("[*] Locating .text section in PE headers...\n");

    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)pe_buf;
    IMAGE_NT_HEADERS64 *nt = (IMAGE_NT_HEADERS64 *)(pe_buf + dos->e_lfanew);
    IMAGE_SECTION_HEADER *sections = (IMAGE_SECTION_HEADER *)(
        (uint8_t *)&nt->OptionalHeader + nt->FileHeader.SizeOfOptionalHeader);

    int patched = 0;
    for (int i = 0; i < nt->FileHeader.NumberOfSections; i++) {
        char name[9] = {0};
        memcpy(name, sections[i].Name, 8);
        logf("    Section %d: '%s'\n", i, name);
        if (strncmp(name, ".text", 5) == 0) {
            logf("[+] Found .text at section %d. Replacing with expanded mutated code (size %zu)...\n", i, write_offset);
            sections[i].SizeOfRawData = write_offset;
            sections[i].Misc.VirtualSize = write_offset;
            memcpy(pe_buf + sections[i].PointerToRawData, mutated, write_offset);
            patched = 1;
            break;
        }
    }

    if (!patched) {
        fprintf(stderr, "[x] Could not patch .text section in output PE\n");
        free(pe_buf);
        free(mutated);
        free(text_data);
        fclose(f_out);
        return 1;
    }

    fwrite(pe_buf, 1, pe_size, f_out);
    fclose(f_out);

    free(pe_buf);
    free(mutated);
    free(text_data);

    logf("[DONE] Wrote patched PE to '%s'\n", output_pe);
    if (log_file) fclose(log_file);
    return 0;
}
