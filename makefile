srcdir = .
abs_top_builddir = .
abs_top_srcdir = .
TOP = $(abs_top_builddir)
ATOP = $(abspath $(TOP))
-include Makefile.conf
export prefix

OS = $(shell uname -s)
ifeq ($(OS),Darwin)
SHLIB_EXT = dylib
# /usr/bin/tic can be too old
EXTRA_NC_CONFIGURE_FLAGS = \
    --with-tic-path=/usr/local/opt/ncurses/bin/tic \
    --with-build-cppflags=-DHAVE_ALLOCA_H=1
else
SHLIB_EXT = so
EXTRA_NC_CONFIGURE_FLAGS =
endif

DJLIBC32 = $(TOP)/lib/libc32_s.a
DJCRT0 = $(TOP)/lib/crt0.elf
DJUCRT0 = $(TOP)/lib/uplt.o
DJ64LIB = $(TOP)/lib/libdj64.*.*.*
DJ64LIBV = $(TOP)/lib/libdj64.*.*
DJ64DEVL = $(TOP)/lib/libdj64.$(SHLIB_EXT)
DJ64LIBS = $(TOP)/lib/libdj64_s.a
DJ32LIBS = $(TOP)/lib/libdj32_s.a
DJDEV64LIB = $(TOP)/lib/libdjdev64.*.*.*
DJDEV64LIBV = $(TOP)/lib/libdjdev64.*.*
DJDEV64DEVL = $(TOP)/lib/libdjdev64.$(SHLIB_EXT)
DJSTUB64LIB = $(TOP)/lib/libdjstub64.*.*.*
DJSTUB64LIBV = $(TOP)/lib/libdjstub64.*.*
DJSTUB64DEVL = $(TOP)/lib/libdjstub64.$(SHLIB_EXT)
DJELFLOAD = $(TOP)/lib/elfload.com
NC_BUILD = contrib/ncurses/build
NC_BUILD32 = contrib/ncurses/build32

.PHONY: dj64 djdev64 demos ncurses ncurses32

all: Makefile.conf dj64 djdev64 ncurses ncurses32
	@echo
	@echo "Done building. You may need to run \"sudo make install\" now."
	@echo "You can first run \"sudo make uninstall\" to purge the prev install."

Makefile.conf config.status: $(abs_top_srcdir)/Makefile.conf.in $(abs_top_srcdir)/configure
	$(abs_top_srcdir)/configure

$(abs_top_srcdir)/configure: $(abs_top_srcdir)/configure.ac
	cd $(@D) && autoreconf -v -i -I m4

subs:
# New makes have a way to avoid parallel invocation with the use of &:
need = 4.3
ifneq ($(filter $(need),$(firstword $(sort $(MAKE_VERSION) $(need)))),)
$(DJ64DEVL) $(DJ32LIBS) $(DJLIBC32) &: subs
else
.NOTPARALLEL:
$(DJ64DEVL) $(DJ32LIBS) $(DJLIBC32) : subs
endif
	$(MAKE) -C src

%.pc: %.pc.in config.status
	./config.status

djdev64: djdev64.pc djstub64.pc
	$(MAKE) -C src/djdev64

ifeq ($(USE64),1)
DJ64PC = dj64.pc dj64_s.pc
endif
DJ64PC += dj32.pc
dj64: $(DJ64PC) $(DJ64DEVL)

ifeq ($(HOST),SunOS)
CP_FP = cp -f
else
CP_FP = cp -fP
endif

install_dj64:
ifeq ($(USE64),1)
	$(INSTALL) -d $(DESTDIR)$(sysroot)/lib
	$(INSTALL) -m 0644 $(DJCRT0) $(DESTDIR)$(sysroot)/lib
	$(INSTALL) -m 0644 $(DJUCRT0) $(DESTDIR)$(sysroot)/lib
	$(INSTALL) -d $(DESTDIR)$(sysroot)/bin
	$(INSTALL) -m 0644 $(DJELFLOAD) $(DESTDIR)$(sysroot)/bin
	$(INSTALL) -d $(DESTDIR)$(sysroot)/lib64
	$(INSTALL) $(DJ64LIB) $(DESTDIR)$(sysroot)/lib64
	$(CP_FP) $(DJ64LIBV) $(DESTDIR)$(sysroot)/lib64
	$(CP_FP) $(DJ64DEVL) $(DESTDIR)$(sysroot)/lib64
	$(INSTALL) -m 0644 $(DJ64LIBS) $(DESTDIR)$(sysroot)/lib64
	$(INSTALL) -d $(DESTDIR)$(sysroot)/include
	cp -r $(abs_top_srcdir)/include $(DESTDIR)$(sysroot)
	$(INSTALL) -d $(DESTDIR)$(sysroot)/share
	$(INSTALL) -m 0644 $(abs_top_srcdir)/dj64.mk $(DESTDIR)$(sysroot)/share
	$(INSTALL) -d $(DESTDIR)$(datadir)
	$(INSTALL) -d $(DESTDIR)$(libdir)/pkgconfig
	$(INSTALL) -m 0644 dj64.pc $(DESTDIR)$(libdir)/pkgconfig
	$(INSTALL) -m 0644 dj64_s.pc $(DESTDIR)$(libdir)/pkgconfig
ifeq ($(NCURSES),1)
	$(MAKE) -C $(NC_BUILD) install
endif
endif

install_djdev64:
	$(INSTALL) -d $(DESTDIR)$(datadir)
	$(INSTALL) -d $(DESTDIR)$(libdir)/pkgconfig
	$(INSTALL) -m 0644 djdev64.pc $(DESTDIR)$(libdir)/pkgconfig
	$(INSTALL) -m 0644 djstub64.pc $(DESTDIR)$(libdir)/pkgconfig
	$(INSTALL) -d $(DESTDIR)$(includedir)/djdev64
	cp -rL $(abs_top_srcdir)/src/djdev64/include/djdev64 $(DESTDIR)$(includedir)
	$(INSTALL) -d $(DESTDIR)$(libdir)
	$(INSTALL) -m 0755 $(DJDEV64LIB) $(DESTDIR)$(libdir)
	$(CP_FP) $(DJDEV64LIBV) $(DESTDIR)$(libdir)
	$(CP_FP) $(DJDEV64DEVL) $(DESTDIR)$(libdir)
	$(INSTALL) -m 0755 $(DJSTUB64LIB) $(DESTDIR)$(libdir)
	$(CP_FP) $(DJSTUB64LIBV) $(DESTDIR)$(libdir)
	$(CP_FP) $(DJSTUB64DEVL) $(DESTDIR)$(libdir)

install: install_dj64 install_djdev64 install_demos install32
	@echo
	@echo "Done installing. You may need to run \"sudo ldconfig\" now."

uninstall64:
ifeq ($(USE64),1)
ifeq ($(NCURSES),1)
ifneq ($(wildcard $(NC_BUILD)),)
	$(MAKE) -C $(NC_BUILD) uninstall
endif
endif
	$(RM) -r $(DESTDIR)$(sysroot)
	$(RM) -r $(DESTDIR)$(includedir)/djdev64
	$(RM) $(DESTDIR)$(libdir)/pkgconfig/dj64.pc
	$(RM) $(DESTDIR)$(libdir)/pkgconfig/dj64_s.pc
	$(MAKE) -C demos src_uninstall
endif

uninstall_dev64:
	$(RM) $(DESTDIR)$(libdir)/pkgconfig/djdev64.pc
	$(RM) $(DESTDIR)$(libdir)/pkgconfig/djstub64.pc
	$(RM) $(DESTDIR)$(libdir)/$(notdir $(DJDEV64DEVL))
	$(RM) $(DESTDIR)$(libdir)/$(notdir $(DJDEV64LIBV))
	$(RM) $(DESTDIR)$(libdir)/$(notdir $(DJDEV64LIB))
	$(RM) $(DESTDIR)$(libdir)/$(notdir $(DJSTUB64DEVL))
	$(RM) $(DESTDIR)$(libdir)/$(notdir $(DJSTUB64LIBV))
	$(RM) $(DESTDIR)$(libdir)/$(notdir $(DJSTUB64LIB))
	ldconfig

install32:
ifeq ($(USE32),1)
	$(INSTALL) -d $(DESTDIR)$(sysroot32)/lib
	$(INSTALL) -d $(DESTDIR)$(sysroot32)/lib32
	$(INSTALL) -m 0644 $(DJLIBC32) $(DESTDIR)$(sysroot32)/lib
	$(INSTALL) -m 0644 $(DJ32LIBS) $(DESTDIR)$(sysroot32)/lib32
	$(INSTALL) -d $(DESTDIR)$(libdir)/pkgconfig
	$(INSTALL) -m 0644 dj32.pc $(DESTDIR)$(libdir)/pkgconfig
	$(INSTALL) -d $(DESTDIR)$(sysroot32)/include
	cp -r $(abs_top_srcdir)/include $(DESTDIR)$(sysroot32)
	$(INSTALL) -d $(DESTDIR)$(sysroot32)/share
	$(INSTALL) -m 0644 $(abs_top_srcdir)/dj32.mk $(DESTDIR)$(sysroot32)/share
ifeq ($(NCURSES),1)
	$(MAKE) -C $(NC_BUILD32) install
endif
endif

uninstall32:
ifeq ($(USE32),1)
ifeq ($(NCURSES),1)
ifneq ($(wildcard $(NC_BUILD32)),)
	$(MAKE) -C $(NC_BUILD32) uninstall
endif
endif
	$(RM) -r $(DESTDIR)$(sysroot32)
	$(RM) $(DESTDIR)$(libdir)/pkgconfig/dj32.pc
endif

uninstall: uninstall64 uninstall_dev64 uninstall32

clean: demos_clean
	$(MAKE) -C src clean
	$(MAKE) -C src/djdev64 clean
	$(RM) *.pc
	$(RM) -r lib
ifeq ($(NCURSES),1)
	$(RM) -r $(NC_BUILD)
	$(RM) -r $(NC_BUILD32)
endif

deb:
	debuild -i -us -uc -b

rpm:
	$(MAKE) clean
	rpkg local && $(MAKE) clean >/dev/null

demos:
	$(MAKE) -C demos

demos_static:
	$(MAKE) -C demos static

demos_djgpp:
	$(MAKE) -C demos djgpp

demos_clean:
	$(MAKE) -C demos clean

install_demos:
ifeq ($(USE64),1)
	$(MAKE) -C demos src_install
endif

ifeq ($(NCURSES),1)
ifeq ($(USE64),1)
L_CPPFLAGS = $(shell PKG_CONFIG_PATH=$(ATOP) pkg-config --variable=xcppflags --define-variable=dj64prefix=$(abs_top_srcdir) dj64)
L_CFLAGS = $(shell PKG_CONFIG_PATH=$(ATOP) pkg-config --cflags dj64)
L_LIBS = $(shell PKG_CONFIG_PATH=$(ATOP) pkg-config --libs-only-L --libs-only-l --define-variable=libdir=$(ATOP)/lib dj64)
R_PREFIX = $(shell PKG_CONFIG_PATH=$(ATOP) pkg-config --variable=dj64prefix dj64)
R_LIBDIR = $(shell PKG_CONFIG_PATH=$(ATOP) pkg-config --variable=libdir dj64)
L_LDFLAGS = $(shell PKG_CONFIG_PATH=$(ATOP) pkg-config --libs-only-other dj64) \
  -Wl,-rpath=$(R_LIBDIR) -nostdlib

$(NC_BUILD) $(NC_BUILD32):
	mkdir -p $@

$(NC_BUILD)/Makefile: dj64.pc | $(NC_BUILD) $(DJ64DEVL)
	cd $(NC_BUILD) && \
	  CC="$(CC)" \
	  LD="$(CC)" \
	  CPPFLAGS="$(L_CPPFLAGS)" \
	  CFLAGS="$(CFLAGS) $(L_CFLAGS)" \
	  LIBS="$(L_LIBS)" \
	  LDFLAGS="$(L_LDFLAGS)" \
  $(abs_top_srcdir)/contrib/ncurses/configure --host=$(shell uname -m)-pc-linux-gnu \
    --prefix=$(R_PREFIX) \
    --libdir=$(R_LIBDIR) \
    --without-manpages \
    --without-cxx \
    --without-ada \
    --without-debug \
    --with-fallbacks=vt100,ansi,cygwin,linux,djgpp,djgpp203,djgpp204 \
    --disable-database \
    --without-tests \
    --without-progs \
    $(EXTRA_NC_CONFIGURE_FLAGS)

ncurses: $(NC_BUILD)/Makefile
	$(MAKE) -C $(NC_BUILD)
else
ncurses:
endif

ifeq ($(USE32),1)
L_CPPFLAGS32 = $(shell PKG_CONFIG_PATH=$(ATOP) pkg-config --variable=xcppflags --define-variable=dj32prefix=$(abs_top_srcdir) dj32)
L_CFLAGS32 = $(shell PKG_CONFIG_PATH=$(ATOP) pkg-config --cflags dj32)
L_LIBS32 = $(shell PKG_CONFIG_PATH=$(ATOP) pkg-config --libs-only-L --libs-only-l --define-variable=libdir32=$(ATOP)/lib dj32)
R_PREFIX32 = $(shell PKG_CONFIG_PATH=$(ATOP) pkg-config --variable=dj32prefix dj32)
R_LIBDIR32 = $(shell PKG_CONFIG_PATH=$(ATOP) pkg-config --variable=libdir32 dj32)
L_LDFLAGS32 = $(shell PKG_CONFIG_PATH=$(ATOP) pkg-config --define-variable=libdir=$(ATOP)/lib --libs-only-other dj32)
$(NC_BUILD32)/Makefile: dj32.pc | $(NC_BUILD32) $(DJ32LIBS)
	cd $(NC_BUILD32) && \
	  CC="$(CC32)" \
	  CPPFLAGS="$(L_CPPFLAGS32)" \
	  CFLAGS="$(L_CFLAGS32) $(CROSS_CFLAGS)" \
	  LIBS="$(L_LIBS32)" \
	  LDFLAGS="$(L_LDFLAGS32)" \
  $(abs_top_srcdir)/contrib/ncurses/configure --host=i686-linux-gnu \
    --prefix=$(R_PREFIX32) \
    --libdir=$(R_LIBDIR32) \
    --without-manpages \
    --without-cxx \
    --without-ada \
    --without-debug \
    --with-fallbacks=vt100,ansi,cygwin,linux,djgpp,djgpp203,djgpp204 \
    --disable-database \
    --without-tests \
    --without-progs \
    $(EXTRA_NC_CONFIGURE_FLAGS)

ncurses32: $(NC_BUILD32)/Makefile
	$(MAKE) -C $(NC_BUILD32)
else
ncurses32:
endif
else
ncurses:
ncurses32:
endif
