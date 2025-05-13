#ifndef MBA_H
#define MBA_H

#include "registers.h"
#include "operand_type.h"
#include "engine.h"
#include "instruction.h" 
#include <stdlib.h>     

uint64_t obfuscate_mba_64(uint64_t imm);
uint32_t obfuscate_mba_32(uint32_t imm);

#endif // MBA_H