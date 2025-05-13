#ifndef MBA_H
#define MBA_H

#include "registers.h"
#include "operand_type.h"
#include "engine.h"
#include "instruction.h" 
#include <stdlib.h>     
#include <string.h>

void xor_decomposition(struct Instruction *out, int target_reg, int temp_reg, uint32_t imm);

#endif // MBA_H