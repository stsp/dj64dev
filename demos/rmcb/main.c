#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <sys/segments.h>
#include "asm.h"
#include "mouse.h"

int main()
{
    int ok;

    /* our rm cb needs to know DS */
    _rmcb_ds = _my_ds();

    ok = mouse_init();
    if (!ok)
        return 1;
    puts("\nclick mouse buttons and scroll the wheel\n");
    puts("press any key to exit");
    getch();
    mouse_done();
    return 0;
}
