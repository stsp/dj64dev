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
#define _GNU_SOURCE
#include <stdio.h>
#include <setjmp.h>
#include <dlfcn.h>
#include "djdev64/djdev64.h"
#include "elf_priv.h"

struct exec_info {
    /* volatile because of longjmp() vs auto storage */
    /* unsigned char to not clash with go64/dj64 errors */
    volatile unsigned char exit_code;
    jmp_buf exit_jmp;
};

struct exec_handle {
    void *dlobj;
    int (*m)(int, char **);
    int (*ae2)(void(*)(void*,int),void*);
};
static struct exec_handle ehands[1];  // expand if more than 1 ever needed

static void exit_exec(void *handle, int rc);

int djdev64_exec(const char *path, int handle, int libid, unsigned flags)
{
    struct exec_handle *eh = &ehands[0];
    dj64init2_t *i2;

    eh->dlobj = NULL;
#ifdef RTLD_DEEPBIND
    eh->dlobj = dlopen(path, RTLD_LOCAL | RTLD_NOW | RTLD_DEEPBIND);
#endif
    if (!eh->dlobj) {
        printf("error loading %s: %s\n", path, dlerror());
        return -1;
    }
    eh->m = dlsym(eh->dlobj, "main");
    if (!eh->m) {
        printf("error: can't find \"main\"\n");
        goto out;
    }
    eh->ae2 = dlsym(eh->dlobj, "atexit2");
    if (!eh->ae2) {
        printf("error: can't find \"atexit2\"\n");
        goto out;
    }
    i2 = dlsym(eh->dlobj, "dj64init2");
    if (!i2) {
        printf("error: can't find \"dj64init2\"\n");
        goto out;
    }

    i2(handle, libid);
    return 0;
out:
    dlclose(eh->dlobj);
    return -1;
}

int djelf64_exec_self(void)
{
    struct exec_handle *eh = &ehands[0];

    eh->dlobj = NULL;
    eh->m = NULL;
#ifdef RTLD_DEFAULT
    eh->m = dlsym(RTLD_DEFAULT, "main");
#endif
    if (!eh->m) {
        printf("error: can't find \"main\"\n");
        return -1;
    }
    eh->ae2 = NULL;
#ifdef RTLD_DEFAULT
    eh->ae2 = dlsym(RTLD_DEFAULT, "atexit2");
#endif
    if (!eh->ae2) {
        printf("error: can't find \"atexit2\"\n");
        return -1;
    }
    return 0;
}

int djelf64_run(int eid, int argc, char **argv)
{
    struct exec_handle *eh;
    struct exec_info ei;
    int ret = -1, rc;

    if (eid != 0)
        return -1;
    eh = &ehands[eid];
    rc = eh->ae2(exit_exec, &ei);
    if (rc == -1) {
        printf("error: atexit2() failed\n");
        goto out;
    }
    if (setjmp(ei.exit_jmp))
        ret = ei.exit_code;
    else
        ret = (unsigned char)eh->m(argc, argv);
    eh->ae2(NULL, NULL);
out:
    if (eh->dlobj)
        dlclose(eh->dlobj);
    return ret;
}

static void exit_exec(void *handle, int rc)
{
    struct exec_info *ei = handle;
    ei->exit_code = rc;
    longjmp(ei->exit_jmp, 1);
}
