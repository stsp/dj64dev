TOP = ..

include $(TOP)/../../Makefile.conf
include $(srcdir)/$(TOP)/../makefile.inc
CPPFLAGS += -I $(srcdir)/$(TOP)/../djdev64/include

P = $(srcdir)/parsers
MP = $(P)/mkproto.sh
GA = $(P)/gen_asms.sh
OS = $(shell uname -s)
ifeq ($(OS),Darwin)
LIBN = libdj64.0.2.dylib
LIBNS = libdj64.dylib
else
LIBN = libdj64.so.0.2
LIBNV = libdj64.so.0
LIBNS = libdj64.so
endif
LIBS = libdj64_s.a
SOURCES = thunks.c thunks_a.c thunks_c.c thunks_p.c dosobj.c
OBJECTS = $(SOURCES:.c=.o)
PLT_S = plt.S
PLT_O = $(PLT_S:.S=.o)

#.INTERMEDIATE: thunk_syms.s thunk_syms.ss
thunk_syms.ss: thunk_syms.c asm_incsn.h
ifeq ($(CC_IS_CLANG),1)
	$(XCPP) -CC -g0 $< | $(CC) -xc - -target i686-unknown-linux-gnu -S $(EXTRA_CFLAGS) -o $@
else
	$(XCPP) -CC -g0 $< | $(CC) -xc - -S $(EXTRA_CFLAGS) -o $@
endif
thunk_syms.s: thunk_syms.ss
	$(P)/sanitize_arch.sh $< >$@
thunk_syms.o: thunk_syms.s
	$(CROSS_AS) $(CROSS_ASFLAGS) $< -o $@_
	$(CROSS_STRIP) --remove-section=.note.gnu.property --strip-debug -o $@ $@_ 2>/dev/null
	$(RM) $@_

ALL_AS_OBJS64 = $(addprefix $(TOP)/,$(file < $(TOP)/makefile.a64))
libc_tmp.a : $(TOP)/makefile.a64 thunk_syms.o $(ALL_AS_OBJS64)
	@-$(RM) $@
	$(CROSS_AR) q $@ $(ALL_AS_OBJS64) thunk_syms.o
	$(CROSS_AR) s $@

asm_incs.h:
	$(GA) 0 $(srcdir)/$(TOP)/../../include >$@

asm_incsn.h:
	$(GA) 1 $(srcdir)/$(TOP)/../../include >$@

asym_incs.h:
	$(GA) 2 $(srcdir)/$(TOP)/../../include >$@

# on Focal we have old make
need = 4.3
ifneq ($(filter $(need),$(firstword $(sort $(MAKE_VERSION) $(need)))),)
thunk_c.h thunk_p.h thunk_incs.h &: libc_tmp.a
else
.NOTPARALLEL:
thunk_c.h thunk_p.h thunk_incs.h : libc_tmp.a
endif
	$(MP) $< $(srcdir)/$(TOP)/../../include thunk_p.h \
		thunk_c.h thunk_incs.h

thunk_calls.tmp: thunk_c.h
	nl -v 0 <$< | sed -E 's/^ *//' >$@
thunk_asms.tmp: thunk_p.h
	nl -v 0 <$< | sed -E 's/^ *//' >$@

TGMK = $(shell $(PKG_CONFIG) --silence-errors --variable=makeinc thunk_gen)
ifeq ($(wildcard $(TGMK)),)
ifeq ($(filter clean,$(MAKECMDGOALS)),)
$(error thunk_gen not installed)
endif
else
TFLAGS = -a 4 -p 4
include $(TGMK)
endif

thunks.C: thunks.c thunk_calls.h thunk_asms.h thunk_incs.h asym_incs.h
thunks_a.C: thunks_a.c asm_incs.h plt_asmc.h asym_incs.h
thunks_c.C: thunks_c.c thunk_incs.h thunk_calls.h
thunks_p.C: thunks_p.c thunk_incs.h thunk_asms.h

$(PLT_O): $(PLT_S) plt.inc $(srcdir)/plt_defs.inc
uplt.o: uplt.S $(srcdir)/plt_defs.inc

ALL_OBJS = $(addprefix $(TOP)/,$(file < $(TOP)/makefile.rf))

OS = $(shell uname -s)
ifeq ($(OS),Darwin)
SHARED_LDFLAGS = $(DJ64_LDFLAGS) -dynamiclib -install_name $(prefix)/i386-pc-dj64/lib64/$(LIBN) -compatibility_version 0.2 -current_version 0.2
else
SHARED_LDFLAGS = $(DJ64_LDFLAGS) -shared -Wl,--version-script,$(srcdir)/libdj64.version -Wl,--no-undefined -Wl,--as-needed -Wl,-soname=$(LIBNV) -Wl,-Bsymbolic
endif

$(LIB)/$(LIBN): $(TOP)/makefile.rf $(ALL_OBJS) \
  thunk_calls.h thunk_asms.h $(OBJECTS) $(srcdir)/libdj64.version
	$(CC) $(SHARED_LDFLAGS) -o $@ \
  $(ALL_OBJS) $(OBJECTS)
	@echo "Have `nm -u $@ | grep "U " | wc -l` undefined symbols"

$(LIB)/$(LIBS): $(TOP)/makefile.rf $(ALL_OBJS) \
  thunk_calls.h thunk_asms.h $(OBJECTS)
	@-$(RM) $@
	$(AR) cr $@ $(ALL_OBJS) $(OBJECTS)
	$(AR) s $@

$(LIB)/libc_s.a: libc_tmp.a $(PLT_O) | $(LIB)
	cp -f $< libctmp.a
	$(CROSS_AR) q libctmp.a $(PLT_O)
	$(CROSS_AR) s libctmp.a
	mv -f libctmp.a $@

$(LIB)/crt0.elf: $(LIB)/libc_s.a
	$(CROSS_LD) --whole-archive $< -melf_i386 -static $(XLD_IMB_OPT)=0x08048000 -o $@
	$(CROSS_STRIP) --strip-debug $@
	chmod -x $@

$(LIB)/uplt.o: uplt.o
	cp $< $@

$(LIB)/$(LIBNS) $(LIB)/$(LIBNV): $(LIB)/$(LIBN) | $(LIB)
	ln -sf $(LIBN) $@

final: $(LIB)/libc_s.a $(LIB)/$(LIBNS) $(LIB)/$(LIBNV) \
  $(LIB)/$(LIBS) $(LIB)/crt0.elf $(LIB)/uplt.o | $(LIB)
	$(MAKE) -C 32 final

clean::
	$(RM) rm *.a *.tmp plt.inc asm_*.h thunk_*.h plt_*.h asym_incs.h
	$(RM) thunk_syms.s thunk_syms.ss
	$(RM) $(LIB)/libc_s.a $(LIB)/libdj64.so* $(LIB)/libdj64.*dylib
	$(MAKE) -C 32 clean
