TOP = ..

include $(TOP)/../makefile.inc
CPPFLAGS += -I ../../djdev64/include

P = parsers
MP = $(P)/mkproto.sh
GA = $(P)/gen_asms.sh
LIBN = libdj64.so.0.2
LIBS = libdj64_s.a
SOURCES = thunks.c thunks_a.c thunks_c.c thunks_p.c dosobj.c smalloc.c
OBJECTS = $(SOURCES:.c=.o)

.INTERMEDIATE: thunk_syms.s thunk_syms.ss
thunk_syms.ss: thunk_syms.c asm_incsn.h
	$(XCPP) -CC -g0 $< | $(CC) -xc - -S $(EXTRA_CFLAGS) -o $@
thunk_syms.s: thunk_syms.ss
	$(P)/sanitize_arch.sh $< >$@
thunk_syms.o: thunk_syms.s
	$(CROSS_AS) --32 $(ASFLAGS) $< -o $@

ALL_AS_OBJS = $(addprefix ../,$(file < $(TOP)/makefile.rfo))
libc_tmp.a : $(TOP)/makefile.rfo thunk_syms.o $(ALL_AS_OBJS)
	@-$(RM) $@
	$(CROSS_AR) q $@ $(ALL_AS_OBJS) thunk_syms.o
	$(CROSS_AR) s $@

asm_incs.h:
	$(GA) 0 $(TOP)/../../include >$@

asm_incsn.h:
	$(GA) 1 $(TOP)/../../include >$@

asym_incs.h:
	$(GA) 2 $(TOP)/../../include >$@

thunk_calls.tmp thunk_asms.tmp thunk_incs.h &: \
    libc_tmp.a
	$(MP) $< $(TOP)/../../include thunk_asms.tmp \
		thunk_calls.tmp thunk_incs.h

TGMK = $(shell pkg-config --silence-errors --variable=makeinc thunk_gen)
ifeq ($(wildcard $(TGMK)),)
ifeq ($(filter clean,$(MAKECMDGOALS)),)
$(error thunk_gen not installed)
endif
else
ifeq ($(filter clean,$(MAKECMDGOALS)),)
$(shell pkg-config --atleast-version=1.1 thunk_gen)
ifneq ($(.SHELLSTATUS),0)
$(error thunk_gen is too old, 1.1 is needed)
endif
endif
TFLAGS = -a 4 -p 4
include $(TGMK)
endif

thunks.o: thunk_calls.h thunk_asms.h thunk_incs.h asym_incs.h
thunks_a.o: asm_incs.h plt_asmc.h asym_incs.h
thunks_c.o: thunk_incs.h thunk_calls.h
thunks_p.o: thunk_incs.h thunk_asms.h

plt.o: plt.S plt.inc plt_defs.inc
uplt.o: uplt.S plt_defs.inc

ALL_OBJS = $(addprefix ../,$(file < $(TOP)/makefile.rf))
$(LIB)/$(LIBN): $(TOP)/makefile.rf $(ALL_OBJS) \
  thunk_calls.h thunk_asms.h $(OBJECTS)
	$(XGCC) -shared -Wl,--no-undefined -Wl,--as-needed -Wl,-soname=$(LIBN) -Wl,-Bsymbolic -o $@ \
  $(ALL_OBJS) $(OBJECTS)
	@echo "Have `nm -u $@ | grep "U " | wc -l` undefined symbols"

$(LIB)/$(LIBS): $(TOP)/makefile.rf $(ALL_OBJS) \
  thunk_calls.h thunk_asms.h $(OBJECTS)
	@-$(RM) $@
	ar q $@ $(ALL_OBJS) $(OBJECTS)
	ar s $@

$(LIB)/libc.a: libc_tmp.a plt.o | $(LIB)
	cp -f $< libctmp.a
	$(CROSS_AR) q libctmp.a plt.o
	$(CROSS_AR) s libctmp.a
	mv -f libctmp.a $@

$(LIB)/crt0.elf: $(LIB)/libc.a
	$(CROSS_LD) --whole-archive $< -melf_i386 -static -o $@
	$(CROSS_STRIP) --strip-debug $@
	chmod -x $@

$(LIB)/uplt.o: uplt.o
	cp $< $@

$(LIB)/libdj64.so: $(LIB)/$(LIBN) | $(LIB)
	ln -sf $(LIBN) $@

final: $(LIB)/libc.a $(LIB)/libdj64.so $(LIB)/$(LIBS) $(LIB)/crt0.elf \
  $(LIB)/uplt.o | $(LIB)

clean::
	$(RM) rm *.a *.tmp plt.inc asm_*.h thunk_*.h plt_*.h asym_incs.h
	$(RM) $(LIB)/libc.a $(LIB)/libdj64.so*
