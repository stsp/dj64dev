TOP ?= ..
prefix ?= /usr/local
INSTALL ?= install

all:
	$(MAKE) -C asmsimp
	$(MAKE) -C file
	$(MAKE) -C hello
	$(MAKE) -C helloasm
	$(MAKE) -C rmcb
	$(MAKE) -C int86

static:
	$(MAKE) -C asmsimp static
	$(MAKE) -C file static
#	$(MAKE) -C hello static
	$(MAKE) -C helloasm static
	$(MAKE) -C rmcb static
	$(MAKE) -C int86 static

djgpp/%:
	$(MAKE) -C asmsimp/djgpp $*
	$(MAKE) -C file/djgpp $*
	$(MAKE) -C hello/djgpp $*
	$(MAKE) -C helloasm/djgpp $*
	$(MAKE) -C rmcb/djgpp $*
	$(MAKE) -C int86/djgpp $*

clean:
	$(MAKE) -C asmsimp clean
	$(MAKE) -C file clean
	$(MAKE) -C hello clean
	$(MAKE) -C helloasm clean
	$(MAKE) -C rmcb clean
	$(MAKE) -C int86 clean

src_install: clean djgpp/clean
	for i in `find . -type d ! -path . -printf '%P\n'`; do \
	  $(INSTALL) -d $(DESTDIR)$(prefix)/share/doc/dj64/demos/$$i; \
	done
	for i in `find . -type f,l ! -path . -printf '%P\n'`; do \
	  $(INSTALL) -m 0644 $$i $(DESTDIR)$(prefix)/share/doc/dj64/demos/$$i; \
	done

src_uninstall:
	$(RM) -r $(DESTDIR)$(prefix)/share/doc/dj64
