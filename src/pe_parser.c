#include "pe_structs.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define TEXT_SECTION_NAME ".text"

/*
PE Header Layout in File:

[DOS Header]                            <-- IMAGE_DOS_HEADER
  ...
  e_lfanew → offset to NT headers

[NT Headers]                            <-- starts at e_lfanew
  [PE Signature]                        <-- 4 bytes ("PE\0\0")
  [IMAGE_FILE_HEADER]                  <-- 20 bytes
  [IMAGE_OPTIONAL_HEADER64]           <-- typically 240 bytes for x64
  [IMAGE_SECTION_HEADER] × N          <-- starts AFTER optional header
*/

int extract_text_section(const char *filename, uint8_t **output, size_t *size, uint32_t *entry_rva) {
    printf("[*] Opening file: %s\n", filename);
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("[x] fopen failed");
        return -1;
    }

    IMAGE_DOS_HEADER dos; 
    fread(&dos, sizeof(dos), 1, f);
    printf("[*] Read DOS header\n");

    if (dos.e_magic != 0x5A4D) {
        printf("[x] Not a valid MZ executable (magic: 0x%04X)\n", dos.e_magic);
        fclose(f);
        return -2;
    }

    printf("[+] Valid MZ header, e_lfanew = 0x%X\n", dos.e_lfanew);

    // Read PE signature (4 bytes) manually
    fseek(f, dos.e_lfanew, SEEK_SET);
    uint32_t pe_signature = 0;
    fread(&pe_signature, sizeof(pe_signature), 1, f);
    if (pe_signature != 0x00004550) {
        printf("[x] Not a valid PE file (signature: 0x%X)\n", pe_signature);
        fclose(f);
        return -3;
    }

    IMAGE_FILE_HEADER file_header;
    fread(&file_header, sizeof(file_header), 1, f);

    IMAGE_OPTIONAL_HEADER64 optional_header;
    fread(&optional_header, sizeof(optional_header), 1, f);

    printf("[+] Valid PE header\n");
    printf("    NumberOfSections: %d\n", file_header.NumberOfSections);
    printf("    SizeOfOptionalHeader: %u\n", file_header.SizeOfOptionalHeader);
    printf("    AddressOfEntryPoint: 0x%X\n", optional_header.AddressOfEntryPoint);
    printf("    ImageBase: 0x%llX\n", optional_header.ImageBase);

    *entry_rva = optional_header.AddressOfEntryPoint;

    // Seek to section table: PE header offset + 4 + file header + optional header
    fseek(f, dos.e_lfanew + 4 + sizeof(IMAGE_FILE_HEADER) + file_header.SizeOfOptionalHeader, SEEK_SET);

    printf("[*] Scanning section headers...\n");

    IMAGE_SECTION_HEADER section;
    for (int i = 0; i < file_header.NumberOfSections; i++) {
        fread(&section, sizeof(section), 1, f);

        char name[9] = {0};
        memcpy(name, section.Name, 8);

        printf("    [%d] Section: '%s'\n", i, name);
        printf("        VirtualSize:       0x%X\n", section.Misc.VirtualSize);
        printf("        VirtualAddress:    0x%X\n", section.VirtualAddress);
        printf("        SizeOfRawData:     0x%X\n", section.SizeOfRawData);
        printf("        PointerToRawData:  0x%X\n", section.PointerToRawData);

        if (strncmp(name, TEXT_SECTION_NAME, 5) == 0) {
            printf("[+] Found .text section at index %d\n", i);
            *size = section.SizeOfRawData;
            *output = malloc(*size);
            if (!*output) {
                printf("[x] malloc failed for .text section\n");
                fclose(f);
                return -5;
            }

            fseek(f, section.PointerToRawData, SEEK_SET);
            size_t read_bytes = fread(*output, 1, *size, f);
            if (read_bytes != *size) {
                printf("[x] Failed to read full .text section (%zu/%zu bytes)\n", read_bytes, *size);
                free(*output);
                fclose(f);
                return -6;
            }

            printf("[+] Successfully extracted .text section (%zu bytes)\n", *size);
            fclose(f);
            return 0;
        }
    }

    printf("[x] .text section not found\n");
    fclose(f);
    return -4;
}
