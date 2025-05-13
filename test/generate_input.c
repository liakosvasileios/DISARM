#include <stdio.h>
#include <stdint.h>

int main() {
    FILE *f = fopen("test_binaries/input.bin", "wb");
    if (!f) {
        perror("fopen");
        return 1;
    }

    // uint8_t code[] = {
    //     0x68, 0x34, 0x12, 0x00, 0x00,             // push 0x1234
    //     0x48, 0x87, 0xD8,                         // xchg rax, rbx
    //     0x48, 0xB8, 0x00, 0x00, 0x00, 0x00,       // mov rax, 0
    //     0x00, 0x00, 0x00, 0x00
    // };

    uint8_t code[] = {
    0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov rax, 0
    0x48, 0x89, 0x18,                                           // mov [rax], rbx
    0x48, 0x05, 0x78, 0x56, 0x34, 0x12,                         // add rax, 0x12345678
    0x48, 0x2D, 0x05, 0x00, 0x00, 0x00,                         // sub rax, 5
    0x48, 0x31, 0xDB,                                           // xor rbx, rbx
    0x68, 0x34, 0x12, 0x00, 0x00,                               // push 0x1234
    0x48, 0x87, 0xD8                                            // xchg rax, rbx
};


    fwrite(code, 1, sizeof(code), f);
    fclose(f);
    printf("[*] input.bin written (%zu bytes)\n", sizeof(code));
    return 0;
}
