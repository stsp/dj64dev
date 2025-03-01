#ifndef GO64_H
#define GO64_H

void pltcall32(__dpmi_regs *regs, __dpmi_paddr addr);
void pltctrl32(__dpmi_regs *regs, int fn, int arg);
void upltinit32(__dpmi_regs *regs);
int elfexec(const char *path, int argc, char **argv);
int elfload(int num);

#define ELFEXEC_LIBID 2

#endif
