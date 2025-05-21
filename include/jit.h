#ifndef JIT_H
#define JIT_H

#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "engine.h"

void* alloc_executable(size_t size);
void emit_virtual_call(uint8_t* out, uint8_t vindex, void** table);

#endif // JIT_H