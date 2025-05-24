#include "pe_parser.h"
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <pe_file>\n", argv[0]);
        return 1;
    }

    PEInfo info;
    if (parse_pe_file(argv[1], &info) != 0) {
        printf("[x] Failed to parse PE file: %s\n", argv[1]);
        return 2;
    }

    printf("[*] Entry point RVA: 0x%X\n", info.entry_rva);
    printf("[*] Image base:      0x%llX\n", info.image_base);
    printf("[*] Parsed %zu target sections:\n", info.num_sections);

    for (size_t i = 0; i < info.num_sections; i++) {
        PESection *s = &info.sections[i];
        printf("  - %s\n", s->name);
        printf("    RVA:        0x%X\n", s->rva);
        printf("    Raw Offset: 0x%X\n", s->raw_offset);
        printf("    Size:       %zu bytes\n", s->size);
    }

    free_pe_info(&info);
    return 0;
}
