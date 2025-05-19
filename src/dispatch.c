#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "engine.h"

#define MAX_FUNCS 4

// Example test functions
__declspec(noinline) void __cdecl vfunc0(void* ctx) { puts("FUNC 0"); }
__declspec(noinline) void __cdecl vfunc1(void* ctx) { puts("FUNC 1"); }
__declspec(noinline) void __cdecl vfunc2(void* ctx) { puts("FUNC 2"); }
__declspec(noinline) void __cdecl vfunc3(void* ctx) { puts("FUNC 3"); }

static void (*real_funcs[MAX_FUNCS])(void*);
static uint8_t index_table[MAX_FUNCS];

// Random shuffle -> 1, deterministic -> 0
void init_dispatch_table(int random) {
    void (*original[])(void*) = { vfunc0, vfunc1, vfunc2, vfunc3 };
    int indices[MAX_FUNCS] = { 0, 1, 2, 3 };
    if (random) {
        srand((unsigned)time(NULL));
    } else {
        srand(1);
    }
    
    for (int i = MAX_FUNCS - 1; i > 0; --i) {
        int j = rand() % (i + 1);
        int tmp = indices[i];
        indices[i] = indices[j];
        indices[j] = tmp;
    }

    for (int i = 0; i < MAX_FUNCS; ++i) {
        real_funcs[i] = original[indices[i]];
        index_table[i] = i; // Optional: shuffle this too
    }
}

void call_virtual(uint8_t vindex) {
    if (vindex >= MAX_FUNCS) {
        puts("invalid vindex");
        return;
    }
    real_funcs[index_table[vindex]](NULL);
}

void* get_dispatch_entry(uint8_t vindex) {
    extern void (*real_funcs[])(void*);
    extern uint8_t index_table[];

    return real_funcs[index_table[vindex]];
}

void** get_dispatch_base() {
    extern void (*real_funcs[])(void*);
    return (void**)real_funcs;
}