#ifndef DISARM_BANNER_H
#define DISARM_BANNER_H

#include <stdio.h>

#define GREEN   "\x1b[32m"
#define CYAN    "\x1b[36m"
#define RED     "\x1b[31m"
#define BOLD    "\x1b[1m"
#define RESET   "\x1b[0m"

static inline void print_disarm_banner() {
        printf(BOLD GREEN 
    "    ____  _________ ___    ____  __  ___\n"
    "   / __ \\/  _/ ___//   |  / __ \\/  |/  /\n"
    "  / / / // / \\__ \\/ /| | / /_/ / /|_/ / \n"
    " / /_/ // / ___/ / ___ |/ _, _/ /  / /  \n"
    "/_____/___//____/_/  |_/_/ |_/_/  /_/   \n"
        RESET);

    printf(BOLD CYAN "\n  DISARM: A Self-Adaptive Metamorphic Engine for Evasion-Driven Binary Mutation\n" RESET);
    printf(RED BOLD "  \"When analysis strikes... DISARM responds.\"\n\n" RESET);
}

#endif  // DISARM_BANNER_H  