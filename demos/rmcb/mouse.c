#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dpmi.h>
#include <sys/farptr.h>
#include <go32.h>
#include "asm.h"
#include "mouse.h"

#define CF 1

#ifdef DJ64
static unsigned int mouse_regs;
#else
static __dpmi_regs *mouse_regs;
#endif

static __dpmi_raddr newm;
static __dpmi_raddr oldm;
static unsigned old_mask;
#define MEV_MASK 0xab

/* avoid using stdio functions from async context */
static void print(const char *msg)
{
    write(STDOUT_FILENO, msg, strlen(msg));
}

static void print_d(const char *msg, int d, const char *msg2)
{
    char buf[16];

    print(msg);
    print(itoa(d, buf, 10));
    print(msg2);
}

static unsigned short popw(__dpmi_regs *r)
{
    unsigned lina = (r->x.ss << 4) + r->x.sp;
    unsigned short ret = _farpeekw(_dos_ds, lina);
    r->x.sp += 2;
    return ret;
}

static void do_retf(__dpmi_regs *r)
{
    r->x.ip = popw(r);
    r->x.cs = popw(r);
}

static void mlb(void)
{
    print("LB\n");
}

static void mrb(void)
{
    print("RB\n");
}

static void mmb(void)
{
    print("MB\n");
}

static void mw(int delta)
{
    print_d("wheel: ", delta, "\n");
}

void do_mouse(void)
{
    __dpmi_regs *r;

#ifdef DJ64
    r = (__dpmi_regs *) DATA_PTR(mouse_regs);
#else
    r = mouse_regs;
#endif
    do_retf(r);

    if (r->x.ax & 2)
	mlb();
    if (r->x.ax & 8)
	mrb();
    if (r->x.ax & 0x20)
	mmb();
    if (r->x.ax & 0x80)
	mw((char ) r->h.bh);
}

int mouse_init(void)
{
    __dpmi_regs r = { };

    __dpmi_int(0x33, &r);
    if ((r.x.flags & CF) || r.x.ax != 0xffff || r.x.bx != 3) {
	puts("mouse not detected");
	return 0;
    }
    /* check the wheel */
    r.x.ax = 0x11;
    __dpmi_int(0x33, &r);
    if ((r.x.flags & CF) || r.x.ax != 0x574d || (r.x.cx & 1) == 0) {
	puts("mouse wheel not supported");
//    return 0;
    }

#ifdef DJ64
    mouse_regs = malloc32(sizeof(__dpmi_regs));
#else
    mouse_regs = (__dpmi_regs *) malloc(sizeof(__dpmi_regs));
#endif
    __dpmi_allocate_real_mode_callback(rmcb_mouse_handler, mouse_regs,
				       &newm);
    r.x.ax = 0x14;
    r.x.cx = MEV_MASK;
    r.x.es = newm.segment;
    r.x.dx = newm.offset16;
    __dpmi_int(0x33, &r);
    oldm.segment = r.x.es;
    oldm.offset16 = r.x.dx;
    old_mask = r.x.cx;

    mouse_show();
    return 1;
}

void mouse_disable(void)
{
    __dpmi_regs r = { };

    mouse_hide();

    r.x.ax = 0x0c;
    r.x.cx = old_mask;
    r.x.es = oldm.segment;
    r.x.dx = oldm.offset16;
    __dpmi_int(0x33, &r);
}

void mouse_done(void)
{
    mouse_disable();
    __dpmi_free_real_mode_callback(&newm);
#ifdef DJ64
    free32(mouse_regs);
#else
    free(mouse_regs);
#endif
}

void mouse_show(void)
{
    __dpmi_regs r = { };

    r.x.ax = 1;
    __dpmi_int(0x33, &r);
}

void mouse_hide(void)
{
    __dpmi_regs r = { };

    r.x.ax = 2;
    __dpmi_int(0x33, &r);
}
