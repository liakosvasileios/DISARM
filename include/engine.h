#ifndef ENGINE_H
#define ENGINE_H

#include <stdint.h>
#include "isa.h"

// Codec macros

#define FETCH_MODRM()   uint8_t modrm = code[offset++]; \
                        uint8_t reg   = (modrm >> 3) & 0x07; \
                        uint8_t rm    = modrm & 0x07;

#define APPLY_REX_BITS()  if (rex & 0x04) reg |= 0x08; \
                          if (rex & 0x01) rm  |= 0x08;

#define FETCH_SIB()      \
    uint8_t sib = code[offset++];              \
    out->scale = (sib >> 6) & 0x03;            \
    out->index = ((sib >> 3) & 0x07)           \
               | ((rex & 0x02) << 2);           \
    out->base  = (sib      & 0x07)             \
               | ((rex & 0x01) << 3);

#define EMIT(byte)        out[offset++] = (byte)

#define ENCODE_MODRM(mod, op1, op2) (((mod) << 6) | (((op2) & 0x07) << 3) | ((op1) & 0x07))

// Codec
int encode_instruction(const struct Instruction* inst, uint8_t* out);
int decode_instruction(const uint8_t* code, struct Instruction* out);

// Dispatch
void init_dispatch_table(int random);
void call_virtual(uint8_t vindex);
void** get_dispatch_base();

#endif  // ENGINE_H