VPATH = ..
CC = i586-pc-msdosdjgpp-gcc
LD = i586-pc-msdosdjgpp-gcc
CFLAGS = -Wall
AS = i586-pc-msdosdjgpp-as
STRIP = i586-pc-msdosdjgpp-strip

SOURCES = hello.c
OBJECTS = $(SOURCES:.c=.o)
TGT = hello.exe

all: $(TGT)

$(TGT): $(OBJECTS)
	$(LD) -o $@ $^
	$(STRIP) $@
	chmod -x $@

clean:
	$(RM) *.o $(TGT)

# display info about the built executable
info: $(TGT)
	djstubify -i $<

run: $(TGT)
	dosemu -dumb $<
