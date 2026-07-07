TOP ?= ..
prefix ?= /usr/local
INSTALL ?= install

DEMOS = asmsimp file hello helloasm rmcb int86
NOSTATIC = hello

all:
	set -e; $(foreach D,$(DEMOS),$(MAKE) -C $(D);)
	set -e; $(foreach D,$(DEMOS),$(MAKE) -C $(D)/32;)

static:
	set -e; $(foreach D,$(filter-out $(NOSTATIC),$(DEMOS)),$(MAKE) -C $(D) static;)

djgpp/%:
	set -e; $(foreach D,$(DEMOS),$(MAKE) -C $(D)/djgpp $*;)

clean:
	$(foreach D,$(DEMOS),$(MAKE) -C $(D) clean;)
	$(foreach D,$(DEMOS),$(MAKE) -C $(D)/32 clean;)

run runelf:
	set -e; $(foreach D,$(DEMOS),$(MAKE) -C $(D) $@;)
	set -e; $(foreach D,$(DEMOS),$(MAKE) -C $(D)/32 $@;)
	set -e; $(foreach D,$(DEMOS),$(MAKE) -C $(D)/djgpp $@;)

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

src_install: clean djgpp/clean
	$(foreach D,$(DEMOS),\
	  $(INSTALL) -d $(DESTDIR)$(prefix)/share/doc/dj64/demos/$(D);\
	  $(INSTALL) -d $(DESTDIR)$(prefix)/share/doc/dj64/demos/$(D)/32;\
	  $(INSTALL) -d $(DESTDIR)$(prefix)/share/doc/dj64/demos/$(D)/djgpp;\
	)
	$(foreach F,$(filter-out %/djgpp %/32,$(call rwildcard,$(DEMOS),*)),\
	  $(INSTALL) -m 0644 $(F) $(DESTDIR)$(prefix)/share/doc/dj64/demos/$(F);)

src_uninstall:
	$(RM) -r $(DESTDIR)$(prefix)/share/doc/dj64
