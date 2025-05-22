#include <stdio.h>
#include <stdlib.h>
#include "pe_structs.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <pe_file>\n", argv[0]);
        return 1;
    }

    uint8_t *text = NULL;
    size_t size = 0;
    uint32_t entry = 0;

    if (extract_text_section(argv[1], &text, &size, &entry) == 0) {
        printf("[+] Extracted .text section (%zu bytes)\n", size);
        printf("[+] Entry Point RVA: 0x%X\n", entry);
        // You could call decode() on `text` here
        free(text);
    }

    return 0;
}
