// pe_parser.h

#ifndef PE_PARSER_H
#define PE_PARSER_H

#include <stdint.h>
#include <stddef.h>
#include "pe_structs.h"  // for IMAGE_* structures

typedef struct {
    char name[9];          // Section name (null-terminated)
    uint8_t *data;         // Raw section data (malloc'd, can be NULL)
    size_t size;           // Size of raw data
    uint32_t rva;          // Relative Virtual Address
    uint32_t raw_offset;   // Offset in file
} PESection;

typedef struct {
    uint32_t entry_rva;
    uint64_t image_base;
    size_t num_sections;
    PESection *sections;   // Dynamically allocated array of sections
} PEInfo;

int parse_pe_file(const char *filename, PEInfo *info);
void free_pe_info(PEInfo *info);

#endif
