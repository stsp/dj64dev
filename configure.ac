dnl Process this file with autoconf to produce a configure script.
AC_INIT([dj64dev], [0.2])
AC_CONFIG_HEADERS([config.hh])
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_FILES([
  Makefile.conf
  dj64.pc
  dj64_s.pc
  dj64static.pc
  djdev64.pc
  djstub64.pc
])

VERSION="$PACKAGE_VERSION"
AC_SUBST(VERSION)

AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AX_PROG_CC_FOR_BUILD
#AX_PROG_AR
AC_CHECK_TOOL([AR], [ar], [:])
if test -z "$AR" -o "$AR" = ":"; then
  AC_MSG_ERROR(You don't have ar installed)
fi

AC_CHECK_PROGS([PKG_CONFIG], [pkg-config])
if test -z "$PKG_CONFIG"; then
  AC_MSG_ERROR(Install pkg-config and retry.)
fi

AC_PATH_PROGS([TIC], [tic])
if test -z "$TIC"; then
  AC_MSG_ERROR(Install ncurses (or ncurses-bin) and retry.)
fi

PKG_CHECK_MODULES([LIBELF], [libelf],, [AC_MSG_ERROR([libelf not installed])])
PKG_CHECK_MODULES([TG], [thunk_gen >= 1.5],, [
  AC_MSG_ERROR([thunk-gen >= 1.5 not installed])
])

DJ64_CFLAGS="$CFLAGS \
-fno-common \
-fpic \
-nostdinc \
-ggdb3 \
-Og \
-Wall \
-Wcast-qual \
-Werror \
-Wno-parentheses \
-Wmissing-declarations \
-Wmissing-prototypes \
-Wpointer-arith \
-Wshadow \
-Wwrite-strings \
-Wundef \
-Wno-array-bounds \
-Wsign-compare \
-Wold-style-definition"

machine=`$CC -dumpmachine | cut -d- -f1 | sed 's/i.86/i386/'`

AC_MSG_CHECKING([for $CC actually being clang])
if $CC -v 2>&1 | grep 'clang' >/dev/null; then
  AC_MSG_RESULT([yes])
  AC_MSG_CHECKING([for clang >= 3.7])
  clang_ver=`$CC -v 2>&1 | grep version | sed 's/.*version //' | cut -d " " -f 1`
  clang_maj=`echo $clang_ver | cut -d "." -f 1`
  clang_mid=`echo $clang_ver | cut -d "." -f 2`
  if test $clang_maj -ge 3 -a $clang_mid -ge 7 -o $clang_maj -ge 4 ; then
    AC_MSG_RESULT([yes, $clang_ver])
  else
    AC_MSG_RESULT([no, $clang_ver])
    AC_MSG_ERROR([Need clang-3.7 or newer, or use gcc instead])
  fi
  CC_IS_CLANG=1
  DJ64_CFLAGS="$DJ64_CFLAGS \
    -Wno-unneeded-internal-declaration \
    -Wno-unknown-warning-option"
else
  AC_MSG_RESULT([no])
  CC_IS_CLANG=0
  DJ64_CFLAGS="$DJ64_CFLAGS \
    -Wno-error=maybe-uninitialized"

fi
AC_SUBST(CC_IS_CLANG)
AC_SUBST(DJ64_CFLAGS)

if test -n "$CROSS_PREFIX"; then
  AC_PATH_PROG([CROSS_AS], [${CROSS_PREFIX}as])
  if test -z "$CROSS_AS"; then
    AC_MSG_ERROR([${CROSS_PREFIX}as not found, CROSS_PREFIX is wrong])
  fi
else
  AC_PATH_PROGS([CROSS_AS], [i686-linux-gnu-as i386-elf-as])
fi
if test -z "$CROSS_AS"; then
  if test "$CONFIG_HOST" = "Darwin"; then
    AC_MSG_ERROR([Please install i386-elf-binutils from https://github.com/nativeos/homebrew-i386-elf-toolchain])
  fi
  AC_PATH_PROG([CROSS_AS], [x86_64-linux-gnu-as])
  if test -z "$CROSS_AS"; then
    if test "$machine" = "i386" -o "$machine" = "x86_64"; then
      AC_PATH_PROG([CROSS_AS], [as])
      if test -z "$CROSS_AS"; then
        AC_PATH_PROG([CROSS_AS], [clang])
        if test -z "$CROSS_AS"; then
          AC_MSG_ERROR([gnu assembler not found, please install clang or binutils])
        else
          CROSS_ASFLAGS="-m32 -x assembler -c"
        fi
      else
        CROSS_ASFLAGS="--32"
      fi
    else
      AC_PATH_PROG([CROSS_AS], [clang])
      if test -n "$CROSS_AS"; then
        CROSS_ASFLAGS="-x assembler -target i686-unknown-linux-gnu -c"
      else
        AC_MSG_ERROR([386 assembler not found, please install clang or binutils-i686-linux-gnu])
      fi
    fi
  else
    CROSS_ASFLAGS="--32"
  fi
fi
AC_SUBST(CROSS_ASFLAGS)

if test -n "$CROSS_PREFIX"; then
  AC_PATH_PROG([CROSS_LD], [${CROSS_PREFIX}ld])
  if test -z "$CROSS_LD"; then
    AC_MSG_ERROR([${CROSS_PREFIX}ld not found, CROSS_PREFIX is wrong])
  fi
else
  AC_PATH_PROGS([CROSS_LD], [i686-linux-gnu-ld i386-elf-ld x86_64-linux-gnu-ld ld.lld])
fi
if test -z "$CROSS_LD"; then
  if test "$machine" = "i386" -o "$machine" = "x86_64"; then
    AC_PATH_PROGS([CROSS_LD], [ld])
  fi
fi
if test -z "$CROSS_LD"; then
  AC_MSG_ERROR([cross-ld not found])
fi

AC_MSG_CHECKING([ld supports --image-base])
if ! $CROSS_LD --image-base=0x0 -v >/dev/null 2>&1; then
  AC_MSG_RESULT([no, using -Ttext-segment])
  XLD_IMB_OPT=-Ttext-segment
else
  AC_MSG_RESULT([yes])
  XLD_IMB_OPT=--image-base
fi
AC_SUBST(XLD_IMB_OPT)

if test -n "$CROSS_PREFIX"; then
  AC_PATH_PROG([CROSS_AR], [${CROSS_PREFIX}ar])
  if test -z "$CROSS_AR"; then
    AC_MSG_ERROR([${CROSS_PREFIX}ar not found, CROSS_PREFIX is wrong])
  fi
else
  AC_PATH_PROGS([CROSS_AR], [i686-linux-gnu-ar i386-elf-ar x86_64-linux-gnu-ar llvm-ar])
fi
if test -z "$CROSS_AR"; then
  if test "$machine" = "i386" -o "$machine" = "x86_64"; then
    AC_PATH_PROGS([CROSS_AR], [ar])
  fi
fi
if test -z "$CROSS_AR"; then
  AC_MSG_ERROR([cross-ar not found])
fi

if test -n "$CROSS_PREFIX"; then
  AC_PATH_PROG([CROSS_STRIP], [${CROSS_PREFIX}strip])
  if test -z "$CROSS_STRIP"; then
    AC_MSG_ERROR([${CROSS_PREFIX}strip not found, CROSS_PREFIX is wrong])
  fi
else
  AC_PATH_PROGS([CROSS_STRIP], [i686-linux-gnu-strip i386-elf-strip x86_64-linux-gnu-strip llvm-strip])
fi
if test -z "$CROSS_STRIP"; then
  if test "$machine" = "i386" -o "$machine" = "x86_64"; then
    AC_PATH_PROGS([CROSS_STRIP], [strip])
  fi
fi
if test -z "$CROSS_STRIP"; then
  AC_MSG_ERROR([cross-strip not found])
fi

AC_CHECK_PROGS([UCTAGS], [uctags ctags])
AC_CHECK_PROGS([URTAGS], [ureadtags readtags])
if test -z "$UCTAGS" -o -z "$URTAGS"; then
  AC_MSG_ERROR([universal-ctags not installed])
fi
if ! $UCTAGS --version | grep "Universal Ctags" >/dev/null; then
  AC_MSG_ERROR([bsd ctags do not suit, install universal-ctags])
fi

AC_CHECK_TYPES([__float80])
AC_CHECK_FUNCS([dlmopen])
AC_CHECK_DECLS([RTLD_DEEPBIND],,, [[#include <dlfcn.h>]])

AC_OUTPUT
