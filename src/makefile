include ../Makefile.conf

all : config makemake.exe subs

makemake.exe : makemake.c
	$(CC_FOR_BUILD) -O2 -Wall makemake.c -o makemake.exe

subs:
	$(MAKE) -C libc

.PHONY : clean
clean : makemake.exe
	$(MAKE) -C libc clean
	$(RM) gcc.opt makefile.sub
	$(RM) makemake.exe

config:
	$(MAKE) -f makefile.cfg
