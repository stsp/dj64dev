# Hello World demo

This demo shows the basic usage of dj64 tool-chain.
Just build an app from a single C source file into an ELF file.
Note that this demo doesn't support static linking: static linking
is currently only supported for exe targets but not for elfs.

## building

Run `make` to build with dj64, or `make -f makefile.djgpp` to build
with djgpp. Run `make info` to print the information about the built
executable.

## running

`make run`
