include ../Makefile.conf

all: makemake.exe subs

makemake.exe: makemake.c
	$(CC_FOR_BUILD) -O2 -Wall -o $@ $^

subs:
	$(MAKE) -C libc

.PHONY : clean
clean : makemake.exe
	$(MAKE) -C libc clean
	$(RM) makefile.sub
	$(RM) makemake.exe
