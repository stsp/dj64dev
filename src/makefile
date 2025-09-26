include ../Makefile.conf

all: subs

makemake.exe: makemake.c
	$(CC_FOR_BUILD) -O2 -Wall -o $@ $^

subs: makemake.exe
	$(MAKE) -C libc
	$(MAKE) -C dosemu2

.PHONY: clean
clean: makemake.exe
	$(MAKE) -C libc clean
	$(MAKE) -C dosemu2 clean
	$(RM) makefile.sub
	$(RM) makemake.exe
