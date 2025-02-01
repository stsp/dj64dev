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

#include <stdio.h>
#include <setjmp.h>
#include <dlfcn.h>
#include <assert.h>
#include "djdev64/djdev64.h"

#define MAX_PATHS 10
static const char *exec_path[MAX_PATHS];
static int num_exec_path;

struct exec_info {
    /* volatile because of longjmp() vs auto storage */
    /* unsigned char to not clash with go64/dj64 errors */
    volatile unsigned char exit_code;
    jmp_buf exit_jmp;
};

static void exit_exec(void *handle, int rc);

int djdev64_exec(const char *path, unsigned flags, int argc, char **argv)
{
    void *dlobj = NULL;
    int (*m)(int, char **);
    int (*ae2)(void(*)(void*,int),void*);
    struct exec_info ei;
    int ret = -1, rc;

#ifdef RTLD_DEEPBIND
    dlobj = dlopen(path, RTLD_LOCAL | RTLD_NOW | RTLD_DEEPBIND);
#endif
    if (!dlobj) {
        printf("error loading %s: %s\n", path, dlerror());
        return -1;
    }
    m = dlsym(dlobj, "main");
    if (!m) {
        printf("error: can't find \"main\"\n");
        goto out;
    }
    ae2 = dlsym(dlobj, "atexit2");
    if (!ae2) {
        printf("error: can't find \"atexit2\"\n");
        goto out;
    }

    rc = ae2(exit_exec, &ei);
    if (rc == -1) {
        printf("error: atexit2() failed\n");
        goto out;
    }
    if (setjmp(ei.exit_jmp))
        ret = ei.exit_code;
    else
        ret = (unsigned char)m(argc, argv);
    ae2(NULL, NULL);
out:
    dlclose(dlobj);
    return ret;
}

static void exit_exec(void *handle, int rc)
{
    struct exec_info *ei = handle;
    ei->exit_code = rc;
    longjmp(ei->exit_jmp, 1);
}

int djdev64_add_load_path(const char *path)
{
    assert(num_exec_path < MAX_PATHS);
    exec_path[num_exec_path] = path;
    return num_exec_path++;
}

int djdev64_load(int num, unsigned flags)
{
    if (num >= num_exec_path)
        return -1;
    return djdev64_exec(exec_path[num], flags, 0, NULL);
}
