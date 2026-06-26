#include <stdio.h>
#include <string.h>
#include "asm.h"

int main()
{
    int rc;

    printf("Hello from C\n");
    rc = hello_asm();
    if (rc == -1)
        printf("unsupported DPMI server, sorry.\n");
    return 0;
}
