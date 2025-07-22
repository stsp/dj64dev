/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <stddef.h>
#include "dos.h"

static struct dos_ops *dosops;

unsigned __dos_open(const char *pathname, unsigned mode, int *handle)
{
    return dosops->_dos_open(pathname, mode, handle);
}

unsigned __dos_read(int handle, void *buffer, unsigned count, unsigned *numread)
{
    return dosops->_dos_read(handle, buffer, count, numread);
}

unsigned __dos_write(int handle, const void *buffer, unsigned count, unsigned *numwrt)
{
    return dosops->_dos_write(handle, buffer, count, numwrt);
}

unsigned long __dos_seek(int handle, unsigned long offset, int origin)
{
    return dosops->_dos_seek(handle, offset, origin);
}

int __dos_close(int handle)
{
    return dosops->_dos_close(handle);
}

int __dos_link_umb(int on)
{
    return dosops->_dos_link_umb(on);
}

void register_dosops(struct dos_ops *dops)
{
    dosops = dops;
}

void unregister_dosops(void)
{
    dosops = NULL;
}
