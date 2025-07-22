#ifndef DOS_H
#define DOS_H

#include "djdev64/stub.h"

unsigned __dos_open(const char *pathname, unsigned mode, int *handle);
unsigned __dos_read(int handle, void *buffer, unsigned count, unsigned *numread);
unsigned __dos_write(int handle, const void *buffer, unsigned count, unsigned *numwrt);
unsigned long __dos_seek(int handle, unsigned long offset, int origin);
int __dos_close(int handle);
int __dos_link_umb(int on);

void register_dosops(struct dos_ops *dops);
void unregister_dosops(void);

#endif
