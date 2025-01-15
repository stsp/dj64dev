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
	$(MAKE) -C asmsimp/djgpp
	$(MAKE) -C file/djgpp
	$(MAKE) -C hello/djgpp
	$(MAKE) -C helloasm/djgpp
	$(MAKE) -C rmcb/djgpp

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
