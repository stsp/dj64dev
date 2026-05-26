include ../Makefile.conf

all: subs

makemake.exe: makemake.c
	$(CC_FOR_BUILD) -O2 -Wall -o $@ $^

subs: makemake.exe
	$(MAKE) -C libc
ifeq ($(USE64),1)
	$(MAKE) -C dosemu2
endif

.PHONY: clean
clean: makemake.exe
	$(MAKE) -C libc clean
	$(MAKE) -C dosemu2 clean
	$(RM) makefile.sub
	$(RM) makemake.exe
