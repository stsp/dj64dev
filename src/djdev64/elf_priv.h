#include <stdint.h>

void *djelf_open_dyn(int fd);
uint32_t djelf_getsymoff(void *arg, const char *name);
void *djelf_open(char *addr, uint32_t size);
void djelf_close(void *arg);
int djelf_reloc(void *arg, uint8_t *addr, uint32_t size, uint32_t va,
        uint32_t *r_entry);

char *djelf64_parse_fd(int fd, uint32_t *r_size);
int djelf64_exec_self(void);
