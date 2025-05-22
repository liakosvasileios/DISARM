#include <stdio.h>
#include <stdlib.h>
#include "disarm_banner.h"

void wait_enter() {
    printf("\nPress Enter to return to menu...");
    getchar(); getchar();
}

void show_menu() {
    int choice;
    do {
        system("clear");
        print_disarm_banner();
        printf("Select an option:\n");
        printf("  [1] Transform binary\n");
        printf("  [2] Run codec test\n");
        printf("  [3] Run mutations test\n");
        printf("  [4] Generate input binary\n");
        printf("  [5] Run JIT engine test\n");
        printf("  [6] Run dispatch table test\n");
        printf("  [7] Deepfry output.bin (set n)\n");
        printf("  [8] Dump disassembly\n");
        printf("  [0] Exit\n");
        printf("> ");
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n'); // clean input
            choice = -1;
        }

        switch (choice) {
            case 1:  system("make transform"); break;
            case 2:  system("make test-decode"); break;
            case 3:  system("make test-mutate"); break;
            case 4:  system("make input"); break;
            case 5:  system("make jit"); break;
            case 6:  system("make test-dispatch"); break;
            case 7:  system("make deepfry n=5"); break;
            case 8:  system("make dump"); break;
            case 0:  printf(RED "Exiting...\n"); break;
            default: printf("RED Invalid choice.\n"); break;
        }

        if (choice != 0) wait_enter();
    } while (choice != 0);
}

int main() {
    show_menu();
    return 0;
}
