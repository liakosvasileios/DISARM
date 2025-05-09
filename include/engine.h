#ifndef ENGINE_H
#define ENGINE_H

#include <stdint.h>
#include "instruction.h"

int encode_instruction(const struct Instruction* inst, uint8_t* out);
int decode_instruction(const uint8_t* code, struct Instruction* out);

#endif  // ENGINE_H