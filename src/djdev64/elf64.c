/*
 *  ELF64 parser for dj64dev.
 *  Copyright (C) 2025  @stsp
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
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
#include <libelf.h>
#include <gelf.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "djdev64/djdev64.h"
#include "elf_priv.h"

static const char *sec_name = ".dj64startup";

static char *do_parse(int fd, uint32_t *r_size)
{
    Elf *e;
    char *ret;
    char *name;
    Elf_Scn *scn;
    GElf_Shdr shdr;
    Elf_Data *data;
    size_t shstrndx;

    if (elf_version(EV_CURRENT) == EV_NONE)
        return NULL;
    if ((e = elf_begin(fd, ELF_C_READ, NULL)) == NULL)
        return NULL;
    if (elf_kind(e) != ELF_K_ELF)
        goto err2;
    if (elf_getshdrstrndx(e, &shstrndx) != 0)
        goto err2;
    scn = NULL;
    while ((scn = elf_nextscn(e, scn)) != NULL) {
        gelf_getshdr(scn, &shdr);
        if (shdr.sh_type != SHT_PROGBITS)
            continue;
        name = elf_strptr(e, shstrndx, shdr.sh_name);
        if (name && strcmp(name, sec_name) == 0)
            break;
    }
    if (!scn) {
//        fprintf(stderr, "dj64-specific sections not found\n");
        goto err2;
    }
    data = elf_getdata(scn, NULL);
    if (!data) {
        fprintf(stderr, "failed to get section data\n");
        goto err2;
    }
    *r_size = data->d_size;
    ret = malloc(data->d_size);
    memcpy(ret, data->d_buf, data->d_size);
    elf_end(e);
    return ret;

err2:
    elf_end(e);
    return NULL;
}

char *djelf64_parse(const char *path, uint32_t *r_size)
{
    char *ret;
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd == -1)
        return NULL;
    ret = do_parse(fd, r_size);
    close(fd);
    return ret;
}

char *djelf64_parse_fd(int fd, uint32_t *r_size)
{
    return do_parse(fd, r_size);
}
