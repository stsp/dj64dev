TOP ?= .
PREFIX ?= /usr/local
LIBDIR ?= $(PREFIX)/lib
INSTALL ?= install

VERSION = 0.1
DJLIBC = $(TOP)/lib/libc.a
DJCRT0 = $(TOP)/lib/crt0.elf
DJUCRT0 = $(TOP)/lib/uplt.o
DJ64LIB = $(TOP)/lib/libdj64.so.*.*
DJ64DEVL = $(TOP)/lib/libdj64.so
DJ64LIBS = $(TOP)/lib/libdj64_s.a
DJDEV64LIB = $(TOP)/lib/libdjdev64.so.*.*
DJDEV64DEVL = $(TOP)/lib/libdjdev64.so
DJSTUB64LIB = $(TOP)/lib/libdjstub64.so.*.*
DJSTUB64DEVL = $(TOP)/lib/libdjstub64.so

ifeq ($(filter demos demos_clean clean,$(MAKECMDGOALS)),)
AS = $(CROSS_PREFIX)as
ifeq ($(CROSS_PREFIX),)
CROSS_PREFIX := i686-linux-gnu-
ifeq ($(shell $(AS) --version 2>/dev/null),)
CROSS_PREFIX := x86_64-linux-gnu-
endif
ifeq ($(shell $(AS) --version 2>/dev/null),)
ifneq ($(filter x86_64 amd64,$(shell uname -m)),)
CROSS_PREFIX :=
else
$(error cross-binutils not installed)
endif
endif
else
# CROSS_PREFIX already set
ifeq ($(shell $(AS) --version 2>/dev/null),)
$(error invalid CROSS_PREFIX)
endif
endif
export CROSS_PREFIX

CROSS_PREFIX_GCC := $(CROSS_PREFIX)
GCC = $(CROSS_PREFIX_GCC)gcc
CROSS_PREFIX_GCC := i686-linux-gnu-
ifeq ($(shell $(GCC) --version 2>/dev/null),)
CROSS_PREFIX_GCC := x86_64-linux-gnu-
endif
ifeq ($(shell $(GCC) --version 2>/dev/null),)
ifneq ($(filter x86_64 amd64,$(shell uname -m)),)
CROSS_PREFIX_GCC :=
else
$(error cross-gcc not installed)
endif
endif
export CROSS_PREFIX_GCC
endif

export PREFIX

.PHONY: subs dj64 djdev64 demos

all: dj64 djdev64
	@echo
	@echo "Done building. You may need to run \"sudo make install\" now."
	@echo "You can first run \"sudo make uninstall\" to purge the prev install."

subs:
	$(MAKE) -C src

djdev64: djdev64.pc djstub64.pc
	$(MAKE) -C src/djdev64

dj64: dj64.pc dj64_s.pc dj64static.pc subs

install_dj64:
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/i386-pc-dj64/lib
	$(INSTALL) -m 0644 $(DJLIBC) $(DESTDIR)$(PREFIX)/i386-pc-dj64/lib
	$(INSTALL) -m 0644 $(DJCRT0) $(DESTDIR)$(PREFIX)/i386-pc-dj64/lib
	$(INSTALL) -m 0644 $(DJUCRT0) $(DESTDIR)$(PREFIX)/i386-pc-dj64/lib
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/i386-pc-dj64/lib64
	$(INSTALL) $(DJ64LIB) $(DESTDIR)$(PREFIX)/i386-pc-dj64/lib64
	cp -fP $(DJ64DEVL) $(DESTDIR)$(PREFIX)/i386-pc-dj64/lib64
	$(INSTALL) -m 0644 $(DJ64LIBS) $(DESTDIR)$(PREFIX)/i386-pc-dj64/lib64
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/i386-pc-dj64/include
	cp -r $(TOP)/include $(DESTDIR)$(PREFIX)/i386-pc-dj64
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/i386-pc-dj64/share
	$(INSTALL) -m 0644 dj64.mk $(DESTDIR)$(PREFIX)/i386-pc-dj64/share
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/share
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/share/pkgconfig
	$(INSTALL) -m 0644 dj64.pc $(DESTDIR)$(PREFIX)/share/pkgconfig
	$(INSTALL) -m 0644 dj64_s.pc $(DESTDIR)$(PREFIX)/share/pkgconfig
	$(INSTALL) -m 0644 dj64static.pc $(DESTDIR)$(PREFIX)/share/pkgconfig

install_djdev64:
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/share
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/share/pkgconfig
	$(INSTALL) -m 0644 djdev64.pc $(DESTDIR)$(PREFIX)/share/pkgconfig
	$(INSTALL) -m 0644 djstub64.pc $(DESTDIR)$(PREFIX)/share/pkgconfig
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/include/djdev64
	cp -rL $(TOP)/src/djdev64/include/djdev64 $(DESTDIR)$(PREFIX)/include
	$(INSTALL) -d $(DESTDIR)$(LIBDIR)
	$(INSTALL) -m 0755 $(DJDEV64LIB) $(DESTDIR)$(LIBDIR)
	cp -fP $(DJDEV64DEVL) $(DESTDIR)$(LIBDIR)
	$(INSTALL) -m 0755 $(DJSTUB64LIB) $(DESTDIR)$(LIBDIR)
	cp -fP $(DJSTUB64DEVL) $(DESTDIR)$(LIBDIR)
	@echo
	@echo "Done installing. You may need to run \"sudo ldconfig\" now."

install: install_dj64 install_djdev64 install_demos

uninstall:
	$(RM) -r $(DESTDIR)$(PREFIX)/bin/dj64-gcc
	$(RM) -r $(DESTDIR)$(PREFIX)/i386-pc-dj64
	$(RM) -r $(DESTDIR)$(PREFIX)/include/djdev64
	$(RM) $(DESTDIR)$(PREFIX)/share/pkgconfig/dj64.pc
	$(RM) $(DESTDIR)$(PREFIX)/share/pkgconfig/dj64static.pc
	$(RM) $(DESTDIR)$(PREFIX)/share/pkgconfig/djdev64.pc
	$(RM) $(DESTDIR)$(PREFIX)/share/pkgconfig/djstub64.pc
	$(RM) $(DESTDIR)$(LIBDIR)/$(notdir $(DJDEV64DEVL))
	$(RM) $(DESTDIR)$(LIBDIR)/$(notdir $(DJDEV64LIB))
	$(RM) $(DESTDIR)$(LIBDIR)/$(notdir $(DJSTUB64DEVL))
	$(RM) $(DESTDIR)$(LIBDIR)/$(notdir $(DJSTUB64LIB))
	ldconfig
	$(MAKE) -C demos src_uninstall

clean: demos_clean
	$(MAKE) -C src clean
	$(MAKE) -C src/djdev64 clean
	$(RM) *.pc
	$(RM) -r lib

deb:
	debuild -i -us -uc -b && $(MAKE) clean >/dev/null

rpm:
	make clean
	rpkg local && $(MAKE) clean >/dev/null

%.pc: %.pc.in makefile
	sed \
		-e 's!@PREFIX[@]!$(PREFIX)!g' \
		-e 's!@INCLUDEDIR[@]!$(INCLUDEDIR)!g' \
		-e 's!@LIBDIR[@]!$(LIBDIR)!g' \
		-e 's!@VERSION[@]!$(VERSION)!g' \
		$< >$@

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
