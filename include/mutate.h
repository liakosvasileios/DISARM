#ifndef MUTATE_H
#define MUTATE_H

#include <stdlib.h>  
#include <time.h>
#include <string.h>
#include "instruction.h"
#include "operand_type.h"
#include "registers.h"

#define CHANCE(x) (rand() % 100 < (x))
#define PERC 100

void mutate_opcode(struct Instruction *inst);

/*
    Accepts an input instruction,

    Writes up to max_count mutated instructions to out_list,

    Returns the number of mutated instructions written (0 if no transformation).
*/
int mutate_multi(const struct Instruction *input, struct Instruction *out_list, int max_count);

#endif  // MUTATE_H