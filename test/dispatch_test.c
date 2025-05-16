#include "engine.h"
#include <stdio.h>

int main() {
    printf("Testing dispatch table...\n");

    init_dispatch_table(0);
    call_virtual(0x00);
    call_virtual(0x01);
    call_virtual(0x02);
    call_virtual(0x03);
}