/*
 *  dj64 - 64bit djgpp-compatible tool-chain
 *  Copyright (C) 2021-2024  @stsp
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <sys/segments.h>
#include <io.h>
#include <dpmi.h>
#include <crt0.h>
#include <stubinfo.h>
#include <go64.h>

// https://github.com/vonj/snippets.org/blob/master/strrpbrk.c
static char *strrpbrk(const char *szString, const char *szChars)
{
      const char  *p;
      char        *p0, *p1;

      for (p = szChars, p0 = p1 = NULL; p && *p; ++p)
      {
            p1 = strrchr(szString, *p);
            if (p1 && p1 > p0)
                  p0 = p1;
      }
      return p0;
}

int elfexec(const char *path, int argc, char **argv)
{
    int err, fd, len, errn, eid, ret;
    const char *p;
    unsigned fname;
    __dpmi_paddr api;
    __dpmi_shminfo shmi;
    __dpmi_meminfo dm;
    __dpmi_regs regs;
    int en_dis = !(_crt0_startup_flags & _CRT0_FLAG_NEARPTR);

    if ((_stubinfo->stubinfo_ver >> 16) < 4) {
        fprintf(stderr, "unsupported stub version %i\n",
                _stubinfo->stubinfo_ver);
        return -1;
    }
    if (_stubinfo->flags & 0x40) {
        fprintf(stderr, "unsupported due to static linking\n");
        return -1;
    }
    err = __dpmi_get_vendor_specific_api_entry_point("DJ64", &api);
    if (err) {
        fprintf(stderr, "DJ64 support missing\n");
        return -1;
    }
    memset(&regs, 0, sizeof(regs));
    regs.d.ebx = 3;  // get version
    pltcall32(&regs, api);
    if (regs.d.eax < 3) {
        fprintf(stderr, "unsupported DJ64 version %i\n", regs.d.eax);
        return -1;
    }
    fd = open(path, O_RDONLY | O_BINARY);
    if (fd == -1) {
        fprintf(stderr, "failed to open %s: %s\n", path, strerror(errno));
        return -1;
    }
    len = filelength(fd);
    p = strrpbrk(path, "/\\");
    if (!p)
        p = path;
    fname = malloc32(strlen(p) + 1);
    strcpy(DATA_PTR(fname), p);
    shmi.size_requested = len;
    shmi.name_offset = fname;
    shmi.name_selector = _my_ds();
    err = __dpmi_allocate_shared_memory(&shmi);
    free32(fname);
    if (err) {
        close(fd);
        fprintf(stderr, "Can't allocate shmem for %s\n", p);
        return -1;
    }
    if (en_dis)
        __djgpp_nearptr_enable();
    err = read(fd, djaddr2ptr(shmi.address), len);
    errn = errno;
    if (en_dis)
        __djgpp_nearptr_disable();
    close(fd);
    if (err == -1) {
        fprintf(stderr, "error reading %s: %s\n", path, strerror(errn));
        return -1;
    }
    if (err != len) {
        fprintf(stderr, "read returned %i, need %i\n", err, len);
        return -1;
    }
    dm.handle = shmi.handle;
    dm.address = shmi.address;
    dm.size = shmi.size;
    err = __dpmi_free_physical_address_mapping(&dm);
    assert(!err);

    regs.d.ebx = 2 | (ELFEXEC_LIBID << 16);  // exec
    regs.d.ecx = 0;  // argc - unsupp
    regs.d.edx = 0;  // argv - unsupp
    regs.x.di = shmi.handle & 0xffff;
    regs.x.si = shmi.handle >> 16;
    pltcall32(&regs, api);
    if (regs.x.flags & 1)
        return -1;
    eid = regs.d.eax;
    memset(&regs, 0, sizeof(regs));
    regs.d.ebx = 6;  // memfd
    regs.x.di = shmi.handle & 0xffff;
    regs.x.si = shmi.handle >> 16;
    pltcall32(&regs, api);
    // eax == fd
    regs.d.ebx = _stubinfo->upl_base;
    regs.d.ecx = _stubinfo->upl_size;
    regs.d.edx = _stubinfo->mem_base;
    pltctrl32(&regs, 2, ELFEXEC_LIBID);
    if (!(regs.x.flags & 1)) {
        // eax == uentry
        regs.d.ebx = ELFEXEC_LIBID;
        upltinit32(&regs);
    }
    ret = djelf_run(eid);
    __dpmi_free_shared_memory(shmi.handle);
    /* returning only 16bit AX allows to distinguish with -1 returns above */
    return ret;
}

int elfload(int num)
{
    __dpmi_regs regs;
    int fd;
    int rc = djelf_load(num, ELFEXEC_LIBID, &fd);
    if (rc == -1)
        return -1;
    regs.d.eax = fd;
    regs.d.ebx = _stubinfo->upl_base;
    regs.d.ecx = _stubinfo->upl_size;
    regs.d.edx = _stubinfo->mem_base;
    pltctrl32(&regs, 2, ELFEXEC_LIBID);
    if (!(regs.x.flags & 1)) {
        // eax == uentry
        regs.d.ebx = ELFEXEC_LIBID;
        upltinit32(&regs);
    }
    return djelf_run(rc);
}
