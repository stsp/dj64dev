TOP = ../..

include $(TOP)/../../Makefile.conf
include $(srcdir)/$(TOP)/../makefile.inc
CPPFLAGS += -I $(srcdir)/$(TOP)/../djdev64/include -I $(srcdir)/..

P = $(srcdir)/../parsers
MP = $(P)/mkproto.sh
GA = $(P)/gen_asms.sh
OS = $(shell uname -s)
LIBS = libdj64_s.a
LIBS32 = libdj32_s.a
SOURCES := thunks.c thunks_a.c thunks_c.c thunks_p.c dosobj.c
$(foreach file,$(SOURCES),$(eval vpath $(file) $(srcdir)/..))
SOURCES += dj32.c
OBJECTS = $(notdir $(SOURCES:.c=.O))

#.INTERMEDIATE: thunk_syms.s thunk_syms.ss
thunk_syms.ss: ../thunk_syms.c asm_incsn.h
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

ALL_AS_OBJS32 = $(addprefix $(TOP)/,$(file < $(TOP)/makefile.a32))
libc_tmp.a : $(TOP)/makefile.a32 thunk_syms.o $(ALL_AS_OBJS32)
	@-$(RM) $@
	$(CROSS_AR) q $@ $(ALL_AS_OBJS32) thunk_syms.o
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

thunks.C32: thunks.c thunk_calls.h thunk_asms.h thunk_incs.h asym_incs.h
thunks_a.C32: thunks_a.c asm_incs.h plt_asmc.h asym_incs.h
thunks_c.C32: thunks_c.c thunk_c1.h thunk_c32.h thunk_incs.h thunk_calls.h
thunks_p.C32: thunks_p.c thunk_p1.h thunk_p32.h thunk_incs.h thunk_asms.h

ALL_OBJS = $(addprefix $(TOP)/,$(file < $(TOP)/makefile.r32))

$(LIB)/$(LIBS32): $(TOP)/makefile.rf $(ALL_OBJS) \
  thunk_calls.h thunk_asms.h $(OBJECTS)
	@-$(RM) $@
	$(AR) cr $@ $(ALL_OBJS) $(OBJECTS)
	$(AR) s $@

$(LIB)/libc32_s.a: libc_tmp.a | $(LIB)
	cp -f $< $@

ifeq ($(USE32),1)
final: $(LIB)/libc32_s.a $(LIB)/$(LIBS32) | $(LIB)
else
final:
endif

clean::
	$(RM) rm *.a *.tmp plt.inc asm_*.h thunk_*.h plt_*.h asym_incs.h
	$(RM) thunk_syms.s thunk_syms.ss
	$(RM) $(LIB)/libc_s.a $(LIB)/libdj64.so* $(LIB)/libdj64.*dylib
