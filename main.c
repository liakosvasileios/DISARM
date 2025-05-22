#include <stdio.h>
#include <stdint.h>
#include "isa.h"

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"
#define BOLD    "\x1b[1m"

    
int main() {
    printf("\n");
    printf(BOLD GREEN 
"    ____  _________ ___    ____  __  ___\n"
"   / __ \\/  _/ ___//   |  / __ \\/  |/  /\n"
"  / / / // / \\__ \\/ /| | / /_/ / /|_/ / \n"
" / /_/ // / ___/ / ___ |/ _, _/ /  / /  \n"
"/_____/___//____/_/  |_/_/ |_/_/  /_/   \n"
      RESET);

    printf(BOLD CYAN "\n  DISARM: A Self-Adaptive Metamorphic Engine for Evasion-Driven Binary Mutation\n" RESET);
    printf(RED BOLD "\n  \"When analysis strikes... DISARM responds.\"\n\n" RESET);

    return 0;
}
