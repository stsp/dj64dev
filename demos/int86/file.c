#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <conio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <dos.h>
#include <sys/segments.h>

#ifdef DJ64
#define USE_INT86x 1  // 0 unsupported in this demo
#else
#define USE_INT86x 0  // 0 or 1 suits
#define P_SEG(p) _my_ds()
#define P_OFF(p) (uintptr_t)(p)
#endif

static int do_read(int fd, void *buf, int size)
{
    union REGS rg = {};
    union REGS rg2 = {};
#if USE_INT86x
    struct SREGS sr = {};
#endif

    rg.h.ah = 0x3f;
    rg.w.bx = fd;
    rg.d.ecx = size;
#if USE_INT86x
    sr.ds = P_SEG(buf);
    rg.d.edx = P_OFF(buf);
    int86x(0x21, &rg, &rg2, &sr);
#else
    rg.d.edx = (uintptr_t)buf;
    int86(0x21, &rg, &rg2);
#endif
    if (rg2.w.flags & 1)
        return -1;
    return rg2.d.eax;
}

static int do_write(int fd, void *buf, int size)
{
    union REGS rg = {};
    union REGS rg2 = {};
#if USE_INT86x
    struct SREGS sr = {};
#endif

    rg.h.ah = 0x40;
    rg.w.bx = fd;
    rg.d.ecx = size;
#if USE_INT86x
    sr.ds = P_SEG(buf);
    rg.d.edx = P_OFF(buf);
    int86x(0x21, &rg, &rg2, &sr);
#else
    rg.d.edx = (uintptr_t)buf;
    int86(0x21, &rg, &rg2);
#endif
    if (rg2.w.flags & 1)
        return -1;
    return rg2.d.eax;
}

int main()
{
    char buf[1024];
    int sz;
    int fd = open("art.txt", O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "can't open art.txt: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    clrscr();
    puts("\n");
    sz = do_read(fd, buf, sizeof(buf));
    if (sz > 0)
        do_write(STDOUT_FILENO, buf, sz);
    puts("\n\npress any key to exit");
    getch();
    return EXIT_SUCCESS;
}
