#include "jit.h"

void* alloc_executable(size_t size) {
    printf("[*] Allocating %zu bytes of executable memory...\n", size);
    void* mem = VirtualAlloc(
        NULL, size,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    );
    if (!mem) {
        printf("[!] VirtualAlloc failed\n");
    } else {
        printf("[+] Executable memory allocated at: %p\n", mem);
    }
    return mem;
}

void emit_virtual_call(uint8_t* out, uint8_t vindex, void** table) {
    printf("[*] Emitting obfuscated virtual dispatch...\n");
    printf("    [vindex] = %u\n", vindex);
    printf("    [dispatch table] = %p\n", table);

    uint8_t* cursor = out;

    // MOV RBX, imm64 (table address)
    struct Instruction mov_rbx = {
        .opcode = 0xB8,
        .operand_type = OPERAND_REG | OPERAND_IMM,
        .op1 = RBX_REG,
        .imm = (uint64_t)(uintptr_t)table,
        .rex = 0x48
    };

    int len = encode_instruction(&mov_rbx, cursor);
    printf("    Encoding: MOV RBX, 0x%llX\n", (unsigned long long)(uintptr_t)table);
    printf("    Bytes: ");
    for (int i = 0; i < len; ++i) printf("%02X ", cursor[i]);
    printf("\n");
    cursor += len;

    // MOV EDX, vindex
    struct Instruction mov_edx = {
        .opcode = 0xB8,
        .operand_type = OPERAND_REG | OPERAND_IMM,
        .op1 = EDX_REG,
        .imm = vindex,
        .rex = 0x00
    };

    len = encode_instruction(&mov_edx, cursor);
    printf("    Encoding: MOV EDX, %u\n", vindex);
    printf("    Bytes: ");
    for (int i = 0; i < len; ++i) printf("%02X ", cursor[i]);
    printf("\n");
    cursor += len;

    // MOV RAX, [RBX + RCX * 8]
    printf("    Encoding: MOV RAX, [RBX + RDX * 8]\n");
    uint8_t* before = cursor;
    *cursor++ = 0x48;        // REX.W
    *cursor++ = 0x8B;        // MOV r64, r/m64
    *cursor++ = 0x04;        // ModRM: mod=00, reg=000 (RAX), rm=SIB (100)
    *cursor++ = 0xD3;        // SIB: scale=8, index=RDX (010), base=RBX (011)
    printf("    Bytes: ");
    for (uint8_t* p = before; p < cursor; ++p) {
        printf("%02X ", *p);
    }
    printf("\n");

    // sub rsp, 0x28
    printf("    Encoding: SUB RSP, 0x28 (shadow space + alignment)\n");
    *cursor++ = 0x48;
    *cursor++ = 0x83;
    *cursor++ = 0xEC;
    *cursor++ = 0x28;

    // call rax
    printf("    Encoding: CALL RAX\n");
    *cursor++ = 0xFF;
    *cursor++ = 0xD0;

    // add rsp, 0x28
    printf("    Encoding: ADD RSP, 0x28\n");
    *cursor++ = 0x48;
    *cursor++ = 0x83;
    *cursor++ = 0xC4;
    *cursor++ = 0x28;

    // ret
    printf("    Encoding: RET\n");
    *cursor++ = 0xC3;

    // Final JIT buffer
    size_t total = cursor - out;
    printf("    Total JIT bytes emitted: %zu\n", total);    
    printf("    Final JIT buffer: ");
    for (size_t i = 0; i < total; ++i) {
        printf("%02X ", out[i]);
    }
    printf("\n\n");
}

void emit_direct_call(uint8_t* out, void* target_func) {
    printf("[*] Emitting DIRECT call to: %p\n", target_func);
    uint8_t* cursor = out;

    // mov rax, imm64
    *cursor++ = 0x48;
    *cursor++ = 0xB8;
    memcpy(cursor, &target_func, 8);
    cursor += 8;

    // sub rsp, 0x28
    *cursor++ = 0x48;
    *cursor++ = 0x83;
    *cursor++ = 0xEC;
    *cursor++ = 0x28;

    // call rax
    *cursor++ = 0xFF;
    *cursor++ = 0xD0;

    // add rsp, 0x28
    *cursor++ = 0x48;
    *cursor++ = 0x83;
    *cursor++ = 0xC4;
    *cursor++ = 0x28;

    // ret
    *cursor++ = 0xC3;

    size_t total = cursor - out;
    printf("    Final buffer (%zu bytes): ", total);
    for (size_t i = 0; i < total; ++i)
        printf("%02X ", out[i]);
    printf("\n");
}


// int main() {
//     printf("[*] Initializing dispatch table (deterministic)...\n");
//     init_dispatch_table(0);

//     void **table = get_dispatch_base();
//     printf("[+] Dispatch table base: %p\n", table);
//     for (int i = 0; i < 4; ++i) {
//         printf("    [%d] = %p\n", i, table[i]);
//     }

//     printf("[*] Preparing JIT memory...\n");
//     uint8_t* jit = alloc_executable(64);
//     if (!jit) return 1;

//     printf("[*] Emitting instructions...\n");
//     emit_virtual_call(jit, 3, table); // index 3 = FUNC 3

//     // printf("[>] Target function at index %d: %p\n", 3, table[3]);
//     // printf("[>] Dereferencing to call: ");
//     // ((void(*)(void*))table[3])(NULL);
//     // puts("[+] Manual call succeeded.");

//     // emit_direct_call(jit, table[3]);
//     // ((void(*)(void*))jit)(NULL);

//     printf("[*] Executing JIT-generated obfuscated virtual call:\n");
//     ((void(*)(void*))jit)(NULL);

//     printf("[*] Freeing JIT memory...\n");
//     VirtualFree(jit, 0, MEM_RELEASE);
//     return 0;
// }
