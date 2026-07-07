#include <stdio.h>
#include <stdlib.h>
#include <conio.h>

int main()
{
    char buf[1024];
    FILE *f = fopen("art.txt", "r");
    if (!f) {
        fprintf(stderr, "can't open art.txt\n");
        return EXIT_FAILURE;
    }

    clrscr();
    puts("\n");
    while (fgets(buf, sizeof(buf), f))
        fputs(buf, stdout);
//    puts("\n\npress any key to exit");
//    getch();
    return EXIT_SUCCESS;
}
