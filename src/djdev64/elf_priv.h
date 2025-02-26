#include <stdint.h>

void *djelf_open(char *addr, uint32_t size);
void *djelf_open_dyn(int fd);
void djelf_close(void *arg);
uint32_t djelf_getsymoff(void *arg, const char *name);
