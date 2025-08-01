ifeq ($(filter clean install,$(MAKECMDGOALS)),)
MACH = $(subst -, ,$(shell $(CC) -dumpmachine))
ifeq ($(MACH),)
$(error unknown target architecture)
endif
OS = $(shell uname -s)
NATIVE=0
ifneq ($(filter x86_64 amd64 i686 i386,$(MACH)),)
ifneq ($(OS), Darwin)
NATIVE = 1
endif
endif

# find the suitable cross-assembler
DJ64AS = $(CROSS_PREFIX)as
DJ64ASFLAGS = --32 --defsym _DJ64=1
XSTRIP = $(CROSS_PREFIX)strip --strip-debug
XLD = $(CROSS_PREFIX)ld
XLD_IMB = -Ttext-segment

ifeq ($(CROSS_PREFIX),)
CROSS_PREFIX := i686-linux-gnu-
ifeq ($(shell $(DJ64AS) --version 2>/dev/null),)
CROSS_PREFIX := x86_64-linux-gnu-
ifeq ($(shell $(DJ64AS) --version 2>/dev/null),)
CROSS_PREFIX :=
endif
endif
else ifeq ($(shell $(DJ64AS) --version 2>/dev/null),)
# CROSS_PREFIX already set
$(error invalid CROSS_PREFIX)
endif

ifeq ($(CROSS_PREFIX),)
ifeq ($(NATIVE), 1)

# natively-compiling, as not found
ifeq ($(shell $(DJ64AS) --version 2>/dev/null),)
# found nothing, try built-in assembler
DJ64AS = $(CC) -x assembler
DJ64ASFLAGS = -m32 -Wa,-defsym,_DJ64=1 -c
endif

else

# cross-compiling, as not found
ifeq ($(shell clang --version 2>/dev/null),)
$(error cross-binutils not installed)
endif
DJ64AS = clang -x assembler -target i686-unknown-linux-gnu
DJ64ASFLAGS = -Wa,-defsym,_DJ64=1 -c
ifeq ($(OS), Darwin)
ifeq ($(shell command -v brew &>/dev/null),)
export PATH += :$(shell brew --prefix llvm)/bin
endif
endif
XSTRIP = llvm-strip --strip-debug
ifeq ($(shell $(XSTRIP) --version 2>/dev/null),)
$(error llvm-strip not found)
endif
XLD = ld.lld
XLD_IMB = --image-base

endif
endif

endif # filter clean install

# Override external AS as termux sets it to aarch64-linux-android-clang
# omitting -c. Note that plain as also doesn't work for termux.
AS = $(CC) -x assembler-with-cpp -c

DJ64CFLAGS = $(shell pkg-config --cflags dj64)
XCPPFLAGS = $(shell pkg-config --variable=xcppflags dj64)
ASCPPFLAGS = $(shell pkg-config --variable=cppflags dj64)

LD = $(CC)

ifeq ($(DJ64COMPACT_VA),)
LOADADDR = 0x08148000
else
LOADADDR = 0x08068000
DJ64_XLDFLAGS += -f 0x20
endif

# freebsd's dlopen() ignores link order and binds to libc the symbols
# defined in libdj64.so.
# Termux doesn't seem to have RTLD_DEEPBIND.
# Use static linking as a work-around.
ifneq ($(filter freebsd% android%,$(MACH)),)
DJ64STATIC = 1
endif
ifeq ($(DJ64STATIC),0)
$(error DJ64STATIC must be empty, not 0)
endif
ifeq ($(DJ64STATIC),1)
DJLDFLAGS = $(shell pkg-config --libs dj64_s)
DJ64_XLDFLAGS += -f 0x40
else
RP := -Wl,-rpath,/usr/local/i386-pc-dj64/lib64 \
  -Wl,-rpath,/usr/i386-pc-dj64/lib64
ifneq ($(PREFIX),)
RP += -Wl,-rpath,$(PREFIX)/i386-pc-dj64/lib64
endif
# sort removes duplicates
DJLDFLAGS = $(shell pkg-config --libs dj64) $(sort $(RP)) $(LDFLAGS)
endif
DJ64_XLIB = libtmp.so
ifneq ($(AS_OBJECTS),)
XELF = tmp.o
endif

.INTERMEDIATE: $(DJ64_XLIB) $(XELF) $(XELF).elf tmp.s

ifneq ($(PDHDR),)
HASH := \#
ifneq ($(shell grep "ASMCFUNC" $(PDHDR) | grep -cv "$(HASH)define"),0)
PLT_O = plt.o
endif
endif
GLOB_ASM = $(wildcard glob_asm.h)

ifneq ($(AS_OBJECTS),)
XLDFLAGS = -melf_i386 -static
ifeq ($(DJ64STATIC),1)
XLDFLAGS += $(shell pkg-config --static --libs dj64static)
DJ64_XLDFLAGS += -f 0x4000
else
XLDFLAGS += $(shell pkg-config --variable=crt0 dj64) $(XLD_IMB)=$(LOADADDR)
endif
$(XELF).elf: $(AS_OBJECTS) $(PLT_O)
	$(XLD) $^ $(XLDFLAGS) -o $@
	$(XSTRIP) $@
$(XELF): $(XELF).elf tmp.s
TMPL = tmp_o_elf

# https://unix.stackexchange.com/a/516476/93040
define _script
cat << EOF >tmp.s
// Warning: autogenerated

#ifdef __APPLE__
.section __TEXT,.dj64startup
#else
.section .dj64startup,"a",%progbits
#endif
.globl _binary_$(TMPL)_start
#ifdef __ELF__
.size _binary_$(TMPL)_start,0
#endif
_binary_$(TMPL)_start:
.incbin "$(XELF).elf"
.globl _binary_$(TMPL)_end
#ifdef __ELF__
.size _binary_$(TMPL)_end,0
#endif
_binary_$(TMPL)_end:

#ifdef __ELF__
.section .note.GNU-stack,"",%progbits
#endif
EOF
endef
export script = $(_script)

tmp.s:
	@eval "$$script"
else
ifeq ($(DJ64STATIC),1)
DJ64_XLDFLAGS += -l $(shell pkg-config --variable=crt0 dj64_s) -f 0x4000
else
DJ64_XLDFLAGS += -f 0x80
endif
endif

$(DJ64_XLIB): $(OBJECTS) $(XELF)
	$(LD) $^ $(DJLDFLAGS) -o $@

%.o: %.c
	$(CC) $(DJ64CFLAGS) $(XCPPFLAGS) $(CFLAGS) -I. -o $@ -c $<
%.o: %.S
	$(CPP) -x assembler-with-cpp $(ASCPPFLAGS) $< | \
	    $(DJ64AS) $(DJ64ASFLAGS) -o $@ -
plt.o: plt.inc $(GLOB_ASM)
	echo "#include <dj64/plt.S.inc>" | \
	    $(CPP) -x assembler-with-cpp $(ASCPPFLAGS) -I. - | \
	    $(DJ64AS) $(DJ64ASFLAGS) -o $@ -
thunks_c.o: thunk_calls.h
thunks_p.o: thunk_asms.h plt_asmc.h

ifneq ($(PDHDR),)
ifneq ($(GLOB_ASM),)
$(OBJECTS): glob_asmdefs.h
endif
# hook in thunk-gen - make sure to not do that before defining `all:` target
TGMK = $(shell pkg-config --variable=makeinc thunk_gen)
ifeq ($(wildcard $(TGMK)),)
ifeq ($(filter clean,$(MAKECMDGOALS)),)
$(error thunk_gen not installed)
endif
else # TGMK
ifeq ($(filter clean,$(MAKECMDGOALS)),)
$(shell pkg-config --atleast-version=1.2 thunk_gen)
ifneq ($(.SHELLSTATUS),0)
$(error thunk_gen is too old, 1.2 is needed)
endif
endif # clean
TFLAGS = -a 4 -p 4
include $(TGMK)
endif # TGMK
endif # PDHDR

ifeq ($(filter clean,$(MAKECMDGOALS)),)
LNK_VER = $(lastword $(shell djlink -v))
ifeq ($(LNK_VER),)
$(error djlink is too old)
endif
LINK = djlink
endif # clean

clean_dj64:
	$(RM) $(OBJECTS) $(AS_OBJECTS) plt.o plt.inc *.tmp
	$(RM) thunk_calls.h thunk_asms.h plt_asmc.h glob_asmdefs.h
	$(RM) $(DJ64_XLIB) $(DJHOSTLIB) $(XELF).elf tmp.s host.elf

EXTRAS = $(shell pkg-config --variable=extras dj64)
ifneq ($(wildcard $(EXTRAS)/*.mk),)
include $(EXTRAS)/*.mk
endif
