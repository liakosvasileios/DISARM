#include "pe_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int read_headers(FILE *f, IMAGE_DOS_HEADER *dos, IMAGE_FILE_HEADER *file_header,
                        IMAGE_OPTIONAL_HEADER64 *opt_header, long *section_table_offset) {
    fread(dos, sizeof(*dos), 1, f);
    if (dos->e_magic != 0x5A4D)
        return -1;

    fseek(f, dos->e_lfanew, SEEK_SET);
    uint32_t pe_sig = 0;
    fread(&pe_sig, sizeof(pe_sig), 1, f);
    if (pe_sig != 0x00004550)
        return -2;

    fread(file_header, sizeof(*file_header), 1, f);
    fread(opt_header, sizeof(*opt_header), 1, f);

    *section_table_offset = dos->e_lfanew + 4 + sizeof(IMAGE_FILE_HEADER) + file_header->SizeOfOptionalHeader;
    return 0;
}

static int read_sections(FILE *f, PEInfo *info, long section_offset, IMAGE_FILE_HEADER *file_header) {
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, section_offset, SEEK_SET);

    PESection *sections = calloc(file_header->NumberOfSections, sizeof(PESection));
    if (!sections) return -1;

    for (int i = 0; i < file_header->NumberOfSections; i++) {
        IMAGE_SECTION_HEADER sh;
        long sh_offset = ftell(f);

        if (sh_offset + sizeof(IMAGE_SECTION_HEADER) > file_size) {
            printf("[!] Skipping invalid section header at offset 0x%lX (index %d)\n", sh_offset, i);
            break;
        }

        if (fread(&sh, sizeof(sh), 1, f) != 1) {
            printf("[!] Failed to read section header at index %d\n", i);
            break;
        }

        PESection *sec = &sections[i];
        memset(sec, 0, sizeof(PESection));
        memcpy(sec->name, sh.Name, 8);
        sec->name[8] = '\0';  // ensure null-terminated
        sec->size = sh.SizeOfRawData;
        sec->rva = sh.VirtualAddress;
        sec->raw_offset = sh.PointerToRawData;

        printf("  - Section [%d] '%s'\n", i, sec->name[0] ? sec->name : "(unnamed)");
        printf("      RVA:        0x%X\n", sec->rva);
        printf("      Raw Offset: 0x%X\n", sec->raw_offset);
        printf("      Size:       %zu bytes\n", sec->size);

        // Only read data if it fits in the file and is not zero-sized
        if (sec->size > 0 &&
            sec->raw_offset > 0 &&
            sec->raw_offset + sec->size <= (size_t)file_size) {

            sec->data = malloc(sec->size);
            if (!sec->data) {
                printf("[!] malloc failed for section '%s'\n", sec->name);
                free(sections);
                return -2;
            }

            fseek(f, sec->raw_offset, SEEK_SET);
            if (fread(sec->data, 1, sec->size, f) != sec->size) {
                printf("[!] Failed to read raw data for section '%s'\n", sec->name);
                free(sec->data);
                sec->data = NULL;
            }
        }
    }

    info->sections = sections;
    info->num_sections = file_header->NumberOfSections;
    return 0;
}


int parse_pe_file(const char *filename, PEInfo *info) {
    FILE *f = fopen(filename, "rb");
    if (!f) return -1;

    IMAGE_DOS_HEADER dos;
    IMAGE_FILE_HEADER file_header;
    IMAGE_OPTIONAL_HEADER64 opt_header;
    long section_offset = 0;

    int hdr_result = read_headers(f, &dos, &file_header, &opt_header, &section_offset);
    if (hdr_result != 0) {
        fclose(f);
        return -2;
    }

    info->entry_rva = opt_header.AddressOfEntryPoint;
    info->image_base = opt_header.ImageBase;

    int sec_result = read_sections(f, info, section_offset, &file_header);
    fclose(f);
    return sec_result;
}

void free_pe_info(PEInfo *info) {
    for (size_t i = 0; i < info->num_sections; i++) {
        free(info->sections[i].data);
    }
    free(info->sections);
    info->sections = NULL;
    info->num_sections = 0;
}
