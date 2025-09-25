# Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details
TOP = .

include $(TOP)/../makefile.inc

all:: $(LIB)/elfload.com

elfload.o: elfload.s
elfload.com.elf: elfload.o
	$(CROSS_LD) -melf_i386 -static --section-start .text=0x100 \
	    -e _start16 -o $@ $^
$(LIB)/elfload.com: elfload.com.elf
	$(CROSS_OBJCOPY) -j .text -O binary $< $@
	chmod -x $@

clean::
	$(RM) *.o *.elf $(LIB)/elfload.com
