#ifndef GO64_H
#define GO64_H

void pltcall32(__dpmi_regs *regs, __dpmi_paddr addr);
int elfexec(const char *path, int argc, char **argv);
int elfload(int num);

#endif
