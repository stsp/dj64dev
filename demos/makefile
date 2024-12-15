TOP ?= ..
prefix ?= /usr/local
INSTALL ?= install

all:
	$(MAKE) -C asmsimp
	$(MAKE) -C file
	$(MAKE) -C hello
	$(MAKE) -C helloasm
	$(MAKE) -C rmcb

static:
	$(MAKE) -C asmsimp static
	$(MAKE) -C file static
#	$(MAKE) -C hello static
	$(MAKE) -C helloasm static
	$(MAKE) -C rmcb static

djgpp:
	$(MAKE) -C asmsimp -f makefile.djgpp
	$(MAKE) -C file -f makefile.djgpp
	$(MAKE) -C hello -f makefile.djgpp
	$(MAKE) -C helloasm -f makefile.djgpp
	$(MAKE) -C rmcb -f makefile.djgpp

clean:
	$(MAKE) -C asmsimp clean
	$(MAKE) -C file clean
	$(MAKE) -C hello clean
	$(MAKE) -C helloasm clean
	$(MAKE) -C rmcb clean

src_install: clean
	for i in `find . -maxdepth 1 -type d ! -path . -printf '%P\n'`; do \
	  $(INSTALL) -d $(DESTDIR)$(prefix)/share/doc/dj64/demos/$$i; \
	  $(INSTALL) -m 0644 $$i/* $(DESTDIR)$(prefix)/share/doc/dj64/demos/$$i; \
	done

src_uninstall:
	$(RM) -r $(DESTDIR)$(prefix)/share/doc/dj64
