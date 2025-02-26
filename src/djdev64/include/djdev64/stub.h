/*
 *  dj64 - 64bit djgpp-compatible tool-chain
 *  Copyright (C) 2021-2024  @stsp
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef STUB_H
#define STUB_H

#include <stdarg.h>
#include "djdpmi.h"

struct dos_ops {
  unsigned (*_dos_open)(const char *pathname, unsigned mode, int *handle);
  unsigned (*_dos_read)(int handle, void *buffer, unsigned count, unsigned *numread);
  unsigned (*_dos_write)(int handle, const void *buffer, unsigned count, unsigned *numwrt);
  unsigned long (*_dos_seek)(int handle, unsigned long offset, int origin);
  int (*_dos_close)(int handle);
  int (*_dos_link_umb)(int on);
};

#define DD(r, n, a, ...) \
  r (*__##n) a;
#define DDv(r, n) \
  r (*__##n)(void);
#define vDD(n, a, ...) \
  void (*__##n) a;
#define vDDv(n) \
  void (*__##n)(void);

struct dpmi_ops {
#include "dpmi_inc.h"
};

#undef DD
#undef DDv
#undef vDD
#undef vDDv

struct stub_ret_regs {
  uint16_t fs;
  uint16_t ds;
  uint16_t cs;
  uint32_t eip;
};

int djstub_main(int argc, char *argv[], char *envp[],
    unsigned psp_sel, int ifile, int ver,
    struct stub_ret_regs *regs, char *(*lin2ptr)(unsigned lin),
    struct dos_ops *dosops, struct dpmi_ops *dpmiops,
    void (*do_printf)(int prio, const char *fmt, va_list ap),
    int (*uput)(int));

#endif
