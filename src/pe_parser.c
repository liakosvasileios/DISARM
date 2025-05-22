#include "pe_structs.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define TEXT_SECTION_NAME ".text"

int extract_text_section(const char *filename, uint8_t **output, size_t *size, uint32_t *entry_rva) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("fopen");
        return -1;
    }

    IMAGE_DOS_HEADER dos; 
    fread(&dos, sizeof(dos), 1, f);
    if (dos.e_magic != 0x5A4D) {
        printf("[x] Not a valid MZ executable\n");
        fclose(f);
        return -2;
    }

    fseek(f, dos.e_lfanew, SEEK_SET);
    IMAGE_NT_HEADERS64 nt;
    fread(&nt, sizeof(nt), 1, f);
    if (nt.Signature != 0x4550) {
        printf("[x] Not a valid PE file\n");
        fclose(f);
        return -3;
    }

    *entry_rva = nt.OptionalHeader.AddressOfEntryPoint;

    IMAGE_SECTION_HEADER section;
    for (int i = 0; i < nt.FileHeader.NumberOfSections; i++) {
        fread(&section, sizeof(section), 1, f);
        if (strncmp((char *)section.Name, TEXT_SECTION_NAME, 8) == 0) {
            *size = section.SizeOfRawData;
            *output = malloc(*size);
            fseek(f, section.PointerToRawData, SEEK_SET);
            fread(*output, 1, *size, f);
            fclose(f);
            return 0;
        }
    }

    printf("[x] .text section not found\n");
    fclose(f);
    return -4;
}