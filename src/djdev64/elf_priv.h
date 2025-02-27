#include <stdint.h>

void *djelf_open_dyn(int fd);
uint32_t djelf_getsymoff(void *arg, const char *name);
