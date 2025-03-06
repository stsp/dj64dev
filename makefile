TOP ?= .
include Makefile.conf
export prefix
export PKG_CONFIG_PATH := $(PKG_CONFIG_PATH):$(datarootdir)/pkgconfig:$(libdir)/pkgconfig

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

.PHONY: subs dj64 djdev64 demos

all: Makefile.conf dj64 djdev64
	@echo
	@echo "Done building. You may need to run \"sudo make install\" now."
	@echo "You can first run \"sudo make uninstall\" to purge the prev install."

Makefile.conf config.status: Makefile.conf.in configure
	./configure

configure: configure.ac
	autoreconf -v -i -I m4

subs:
	$(MAKE) -C src

%.pc: %.pc.in config.status
	./config.status

djdev64: djdev64.pc djstub64.pc
	$(MAKE) -C src/djdev64

dj64: dj64.pc dj64_s.pc dj64static.pc dj64host.pc subs

install_dj64:
	$(INSTALL) -d $(DESTDIR)$(prefix)/i386-pc-dj64/lib
	$(INSTALL) -m 0644 $(DJLIBC) $(DESTDIR)$(prefix)/i386-pc-dj64/lib
	$(INSTALL) -m 0644 $(DJCRT0) $(DESTDIR)$(prefix)/i386-pc-dj64/lib
	$(INSTALL) -m 0644 $(DJUCRT0) $(DESTDIR)$(prefix)/i386-pc-dj64/lib
	$(INSTALL) -d $(DESTDIR)$(prefix)/i386-pc-dj64/lib64
	$(INSTALL) $(DJ64LIB) $(DESTDIR)$(prefix)/i386-pc-dj64/lib64
	cp -fP $(DJ64DEVL) $(DESTDIR)$(prefix)/i386-pc-dj64/lib64
	$(INSTALL) -m 0644 $(DJ64LIBS) $(DESTDIR)$(prefix)/i386-pc-dj64/lib64
	$(INSTALL) -d $(DESTDIR)$(prefix)/i386-pc-dj64/include
	cp -r $(TOP)/include $(DESTDIR)$(prefix)/i386-pc-dj64
	$(INSTALL) -d $(DESTDIR)$(prefix)/i386-pc-dj64/share
	$(INSTALL) -m 0644 dj64.mk $(DESTDIR)$(prefix)/i386-pc-dj64/share
	$(INSTALL) -d $(DESTDIR)$(prefix)/share
	$(INSTALL) -d $(DESTDIR)$(prefix)/share/pkgconfig
	$(INSTALL) -m 0644 dj64.pc $(DESTDIR)$(prefix)/share/pkgconfig
	$(INSTALL) -m 0644 dj64_s.pc $(DESTDIR)$(prefix)/share/pkgconfig
	$(INSTALL) -m 0644 dj64static.pc $(DESTDIR)$(prefix)/share/pkgconfig
	$(INSTALL) -m 0644 dj64host.pc $(DESTDIR)$(prefix)/share/pkgconfig

install_djdev64:
	$(INSTALL) -d $(DESTDIR)$(prefix)/share
	$(INSTALL) -d $(DESTDIR)$(prefix)/share/pkgconfig
	$(INSTALL) -m 0644 djdev64.pc $(DESTDIR)$(prefix)/share/pkgconfig
	$(INSTALL) -m 0644 djstub64.pc $(DESTDIR)$(prefix)/share/pkgconfig
	$(INSTALL) -d $(DESTDIR)$(prefix)/include/djdev64
	cp -rL $(TOP)/src/djdev64/include/djdev64 $(DESTDIR)$(prefix)/include
	$(INSTALL) -d $(DESTDIR)$(libdir)
	$(INSTALL) -m 0755 $(DJDEV64LIB) $(DESTDIR)$(libdir)
	cp -fP $(DJDEV64DEVL) $(DESTDIR)$(libdir)
	$(INSTALL) -m 0755 $(DJSTUB64LIB) $(DESTDIR)$(libdir)
	cp -fP $(DJSTUB64DEVL) $(DESTDIR)$(libdir)

install: install_dj64 install_djdev64 install_demos
	@echo
	@echo "Done installing. You may need to run \"sudo ldconfig\" now."

uninstall:
	$(RM) -r $(DESTDIR)$(prefix)/i386-pc-dj64
	$(RM) -r $(DESTDIR)$(prefix)/include/djdev64
	$(RM) $(DESTDIR)$(prefix)/share/pkgconfig/dj64.pc
	$(RM) $(DESTDIR)$(prefix)/share/pkgconfig/dj64_s.pc
	$(RM) $(DESTDIR)$(prefix)/share/pkgconfig/dj64static.pc
	$(RM) $(DESTDIR)$(prefix)/share/pkgconfig/djdev64.pc
	$(RM) $(DESTDIR)$(prefix)/share/pkgconfig/djstub64.pc
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

deb:
	debuild -i -us -uc -b && $(MAKE) clean >/dev/null

rpm:
	make clean
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
