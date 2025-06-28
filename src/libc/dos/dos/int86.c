/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1997 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <libc/stubs.h>
#include <string.h>
#include <dos.h>
#include <go32.h>
#include <dpmi.h>

int int86(int ivec, union REGS *in, union REGS *out)
{
  __dpmi_regs regs = {0};
  memcpy(&regs, in, 34);
  memcpy(out, in, 34);
  regs.x.ds = regs.x.es = __tb / 16;

  if(ivec != 0x21)
    return _int86(ivec, in, out);

  switch (regs.h.ah) {
    case 0x56:
      {
        char *ptr = DATA_PTR(regs.d.edi);
        dosmemput(ptr, strlen(ptr)+1, __tb + 512);
        regs.x.di = 512;
      }
    case 0x09:
    case 0x39:
    case 0x3a:
    case 0x3b:
    case 0x3c:
    case 0x3d:
    case 0x41:
    case 0x43:
      {
        char *ptr = DATA_PTR(regs.d.edx);
        unsigned len;
        if(regs.h.ah == 9)
          for(len=0; ptr[len] != '$'; len++);
        else
          len = strlen(ptr);
        dosmemput(ptr, len+1, __tb);
        regs.x.dx = 0;
        __dpmi_int(0x21, &regs);
        out->d.ecx = regs.x.cx;		/* 0x43 needs this */
        goto doexit;
      }

    case 0x47:
      {
        char *ptr = DATA_PTR(regs.d.esi);
        regs.x.si = 0;
        __dpmi_int(0x21, &regs);
        dosmemget(__tb, 64, ptr);
        goto doexit;
      }

    case 0x3f:
      {
        char *ptr = DATA_PTR(regs.d.edx);
        unsigned count = regs.d.ecx;
        unsigned total = 0;
        regs.x.dx = 0;
        while(count) {
          regs.x.cx = (count <= __tb_size) ? count : __tb_size;
          __dpmi_int(0x21, &regs);
          if(regs.x.flags & 1)
            goto doexit;
          dosmemget(__tb, regs.x.ax, ptr);
          total += regs.x.ax;
          ptr += regs.x.ax;
          count -= regs.x.ax;
          if(regs.x.ax < regs.x.cx)
            break;
        }
        out->d.eax = total;
        out->d.eflags = regs.x.flags;
        out->d.cflag = regs.x.flags & 1;
        return total;
      }

    case 0x40:
      {
        char *ptr = DATA_PTR(regs.d.edx);
        unsigned count = regs.d.ecx;
        unsigned total = 0;
        regs.x.dx = 0;
        do {
          regs.x.cx = (count <= __tb_size) ? count : __tb_size;
          dosmemput(ptr, regs.x.cx, __tb);
          __dpmi_int(0x21, &regs);
          if(regs.x.flags & 1)
            goto doexit;
          dosmemget(__tb, regs.x.ax, ptr);
          total += regs.x.ax;
          ptr += regs.x.ax;
          count -= regs.x.ax;
          if(regs.x.ax < regs.x.cx)
            break;
        } while(count);
        out->d.eax = total;
        out->d.eflags = regs.x.flags;
        out->d.cflag = regs.x.flags & 1;
        return total;
      }

    default:
      return _int86(ivec, in, out);
  }

doexit:
  out->d.eflags = regs.x.flags;
  out->d.cflag = regs.x.flags & 1;
  out->d.eax = regs.x.ax;
  return regs.x.ax;
}

/*
The following interrupts were extended in GO32 and not here:

0x10:
      AH = 0x11 - not here because inconsistent implementation or BP used

0x21:
      (support for findfirst/findnext - structure changed)
      AH = 0x1a, set var dta = EDX, set dta = TB
      AH = 0x2f, set EBX = var dta
      AH = 0x4e, get util nul EDX to TB+43; * put to user_dta 48 bytes
      AH = 0x4f, get user_dta 48 bytes to TB, *, put to user_dta 48 bytes
*/

/* silly copy/paste from int86() above, sorry. -stsp */
int int86x(int ivec, union REGS *in, union REGS *out, struct SREGS *sregs)
{
  __dpmi_regs regs = {0};
  memcpy(&regs, in, 34);
  memcpy(out, in, 34);
  regs.x.ds = regs.x.es = __tb / 16;

  if(ivec != 0x21)
    return _int86x(ivec, in, out, sregs);

  switch (regs.h.ah) {
    case 0x56:
      {
        char *ptr = SEG_P(sregs->es) + regs.d.edi;
        dosmemput(ptr, strlen(ptr)+1, __tb + 512);
        regs.x.di = 512;
      }
    case 0x09:
    case 0x39:
    case 0x3a:
    case 0x3b:
    case 0x3c:
    case 0x3d:
    case 0x41:
    case 0x43:
      {
        char *ptr = SEG_P(sregs->ds) + regs.d.edx;
        unsigned len;
        if(regs.h.ah == 9)
          for(len=0; ptr[len] != '$'; len++);
        else
          len = strlen(ptr);
        dosmemput(ptr, len+1, __tb);
        regs.x.dx = 0;
        __dpmi_int(0x21, &regs);
        out->d.ecx = regs.x.cx;		/* 0x43 needs this */
        goto doexit;
      }

    case 0x47:
      {
        char *ptr = SEG_P(sregs->ds) + regs.d.esi;
        regs.x.si = 0;
        __dpmi_int(0x21, &regs);
        dosmemget(__tb, 64, ptr);
        goto doexit;
      }

    case 0x3f:
      {
        char *ptr = SEG_P(sregs->ds) + regs.d.edx;
        unsigned count = regs.d.ecx;
        unsigned total = 0;
        regs.x.dx = 0;
        while(count) {
          regs.x.cx = (count <= __tb_size) ? count : __tb_size;
          __dpmi_int(0x21, &regs);
          if(regs.x.flags & 1)
            goto doexit;
          dosmemget(__tb, regs.x.ax, ptr);
          total += regs.x.ax;
          ptr += regs.x.ax;
          count -= regs.x.ax;
          if(regs.x.ax < regs.x.cx)
            break;
        }
        out->d.eax = total;
        out->d.eflags = regs.x.flags;
        out->d.cflag = regs.x.flags & 1;
        return total;
      }

    case 0x40:
      {
        char *ptr = SEG_P(sregs->ds) + regs.d.edx;
        unsigned count = regs.d.ecx;
        unsigned total = 0;
        regs.x.dx = 0;
        do {
          regs.x.cx = (count <= __tb_size) ? count : __tb_size;
          dosmemput(ptr, regs.x.cx, __tb);
          __dpmi_int(0x21, &regs);
          if(regs.x.flags & 1)
            goto doexit;
          dosmemget(__tb, regs.x.ax, ptr);
          total += regs.x.ax;
          ptr += regs.x.ax;
          count -= regs.x.ax;
          if(regs.x.ax < regs.x.cx)
            break;
        } while(count);
        out->d.eax = total;
        out->d.eflags = regs.x.flags;
        out->d.cflag = regs.x.flags & 1;
        return total;
      }

    default:
      return _int86x(ivec, in, out, sregs);
  }

doexit:
  out->d.eflags = regs.x.flags;
  out->d.cflag = regs.x.flags & 1;
  out->d.eax = regs.x.ax;
  return regs.x.ax;
}
