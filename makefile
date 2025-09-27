TOP ?= .
ATOP = $(abspath $(TOP))
-include Makefile.conf
export prefix
export PKG_CONFIG_PATH := $(PKG_CONFIG_PATH):$(datadir)/pkgconfig:$(libdir)/pkgconfig

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

DJLIBC = $(TOP)/lib/libc_s.a
DJCRT0 = $(TOP)/lib/crt0.elf
DJUCRT0 = $(TOP)/lib/uplt.o
DJ64LIB = $(TOP)/lib/libdj64.*.*.*
DJ64DEVL = $(TOP)/lib/libdj64.$(SHLIB_EXT)
DJ64LIBS = $(TOP)/lib/libdj64_s.a
DJDEV64LIB = $(TOP)/lib/libdjdev64.*.*.*
DJDEV64DEVL = $(TOP)/lib/libdjdev64.$(SHLIB_EXT)
DJSTUB64LIB = $(TOP)/lib/libdjstub64.*.*.*
DJSTUB64DEVL = $(TOP)/lib/libdjstub64.$(SHLIB_EXT)
DJELFLOAD = $(TOP)/lib/elfload.com
NC_BUILD = contrib/ncurses/build

.PHONY: subs dj64 djdev64 demos ncurses

all: Makefile.conf dj64 djdev64 ncurses
	@echo
	@echo "Done building. You may need to run \"sudo make install\" now."
	@echo "You can first run \"sudo make uninstall\" to purge the prev install."

Makefile.conf config.status: Makefile.conf.in $(abs_top_srcdir)/configure
	$(abs_top_srcdir)/configure

$(abs_top_srcdir)/configure: $(abs_top_srcdir)/configure.ac
	cd $(@D) && autoreconf -v -i -I m4

subs:
	$(MAKE) -C src

%.pc: %.pc.in config.status
	./config.status

djdev64: djdev64.pc djstub64.pc
	$(MAKE) -C src/djdev64

dj64: dj64.pc dj64_s.pc dj64static.pc subs

install_dj64:
	$(INSTALL) -d $(DESTDIR)$(prefix)/i386-pc-dj64/lib
	$(INSTALL) -m 0644 $(DJLIBC) $(DESTDIR)$(prefix)/i386-pc-dj64/lib
	$(INSTALL) -m 0644 $(DJCRT0) $(DESTDIR)$(prefix)/i386-pc-dj64/lib
	$(INSTALL) -m 0644 $(DJUCRT0) $(DESTDIR)$(prefix)/i386-pc-dj64/lib
	$(INSTALL) -d $(DESTDIR)$(prefix)/i386-pc-dj64/bin
	$(INSTALL) -m 0644 $(DJELFLOAD) $(DESTDIR)$(prefix)/i386-pc-dj64/bin
	$(INSTALL) -d $(DESTDIR)$(prefix)/i386-pc-dj64/lib64
	$(INSTALL) $(DJ64LIB) $(DESTDIR)$(prefix)/i386-pc-dj64/lib64
	cp -fP $(DJ64DEVL) $(DESTDIR)$(prefix)/i386-pc-dj64/lib64
	$(INSTALL) -m 0644 $(DJ64LIBS) $(DESTDIR)$(prefix)/i386-pc-dj64/lib64
	$(INSTALL) -d $(DESTDIR)$(prefix)/i386-pc-dj64/include
	cp -r $(abs_top_srcdir)/include $(DESTDIR)$(prefix)/i386-pc-dj64
	$(INSTALL) -d $(DESTDIR)$(prefix)/i386-pc-dj64/share
	$(INSTALL) -m 0644 $(abs_top_srcdir)/dj64.mk $(DESTDIR)$(prefix)/i386-pc-dj64/share
	$(INSTALL) -d $(DESTDIR)$(datadir)
	$(INSTALL) -d $(DESTDIR)$(datadir)/pkgconfig
	$(INSTALL) -m 0644 dj64.pc $(DESTDIR)$(datadir)/pkgconfig
	$(INSTALL) -m 0644 dj64_s.pc $(DESTDIR)$(datadir)/pkgconfig
	$(INSTALL) -m 0644 dj64static.pc $(DESTDIR)$(datadir)/pkgconfig
ifeq ($(NCURSES),1)
	$(MAKE) -C $(NC_BUILD) install
endif

install_djdev64:
	$(INSTALL) -d $(DESTDIR)$(datadir)
	$(INSTALL) -d $(DESTDIR)$(datadir)/pkgconfig
	$(INSTALL) -m 0644 djdev64.pc $(DESTDIR)$(datadir)/pkgconfig
	$(INSTALL) -m 0644 djstub64.pc $(DESTDIR)$(datadir)/pkgconfig
	$(INSTALL) -d $(DESTDIR)$(includedir)/djdev64
	cp -rL $(abs_top_srcdir)/src/djdev64/include/djdev64 $(DESTDIR)$(includedir)
	$(INSTALL) -d $(DESTDIR)$(libdir)
	$(INSTALL) -m 0755 $(DJDEV64LIB) $(DESTDIR)$(libdir)
	cp -fP $(DJDEV64DEVL) $(DESTDIR)$(libdir)
	$(INSTALL) -m 0755 $(DJSTUB64LIB) $(DESTDIR)$(libdir)
	cp -fP $(DJSTUB64DEVL) $(DESTDIR)$(libdir)

install: install_dj64 install_djdev64 install_demos
	@echo
	@echo "Done installing. You may need to run \"sudo ldconfig\" now."

uninstall:
ifeq ($(NCURSES),1)
	$(MAKE) -C $(NC_BUILD) uninstall
endif
	$(RM) -r $(DESTDIR)$(prefix)/i386-pc-dj64
	$(RM) -r $(DESTDIR)$(includedir)/djdev64
	$(RM) $(DESTDIR)$(datadir)/pkgconfig/dj64.pc
	$(RM) $(DESTDIR)$(datadir)/pkgconfig/dj64_s.pc
	$(RM) $(DESTDIR)$(datadir)/pkgconfig/dj64static.pc
	$(RM) $(DESTDIR)$(datadir)/pkgconfig/djdev64.pc
	$(RM) $(DESTDIR)$(datadir)/pkgconfig/djstub64.pc
	$(RM) $(DESTDIR)$(libdir)/$(notdir $(DJDEV64DEVL))
	$(RM) $(DESTDIR)$(libdir)/$(notdir $(DJDEV64LIB))
	$(RM) $(DESTDIR)$(libdir)/$(notdir $(DJSTUB64DEVL))
	$(RM) $(DESTDIR)$(libdir)/$(notdir $(DJSTUB64LIB))
	ldconfig
	$(MAKE) -C demos src_uninstall

clean: demos_clean
	$(MAKE) -C src clean
	$(MAKE) -C src/djdev64 clean
	$(RM) *.pc
	$(RM) -r lib
ifeq ($(NCURSES),1)
	$(RM) -r $(NC_BUILD)
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
	$(MAKE) -C demos src_install

$(DJ64DEVL): subs

ifeq ($(NCURSES),1)
L_CPPFLAGS = $(shell PKG_CONFIG_PATH=$(ATOP) pkg-config --variable=xcppflags --define-variable=dj64prefix=$(abs_top_srcdir) dj64)
L_CFLAGS = $(shell PKG_CONFIG_PATH=$(ATOP) pkg-config --cflags dj64)
L_LIBS = $(shell PKG_CONFIG_PATH=$(ATOP) pkg-config --libs-only-L --libs-only-l --define-variable=libdir=$(ATOP)/lib dj64)
R_PREFIX = $(shell PKG_CONFIG_PATH=$(ATOP) pkg-config --variable=dj64prefix dj64)
R_LIBDIR = $(shell PKG_CONFIG_PATH=$(ATOP) pkg-config --variable=libdir dj64)
L_LDFLAGS = $(shell PKG_CONFIG_PATH=$(ATOP) pkg-config --libs-only-other dj64) \
  -Wl,-rpath=$(R_LIBDIR) -nostdlib
$(NC_BUILD):
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
