dnl Process this file with autoconf to produce a configure script.
AC_INIT([dj64dev], [0.4])
AC_CONFIG_HEADERS([config.hh])
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_FILES([
  Makefile.conf
  dj64.pc
  dj64_s.pc
  djdev64.pc
  djstub64.pc
  dj32.pc
])

VERSION="$PACKAGE_VERSION"
AC_SUBST(VERSION)

AC_PROG_CC
AC_PROG_INSTALL
AX_PROG_CC_FOR_BUILD
#AX_PROG_AR
AC_PROG_LN_S
AC_PROG_SED
AC_PROG_GREP

# termux uses PREFIX var for default prefix
AC_PREFIX_DEFAULT([${PREFIX:-/usr/local}])

AC_CHECK_PROGS([PKG_CONFIG], [pkg-config])
if test -z "$PKG_CONFIG"; then
  AC_MSG_ERROR(Install pkg-config and retry.)
fi

ARCH_DIR=i386-pc-dj64
ARCH_DIR32=i386-pc-dj32
USE64=1
USE32=1

AC_ARG_ENABLE(64bit,
  AS_HELP_STRING([--disable-64bit], [do not build 64bit components (reduced build)]))
if test "$enable_64bit" != "no"; then
  PKG_CHECK_MODULES([LIBELF], [libelf],, [
    AC_CHECK_LIB(elf, elf_memory,, [
      AC_MSG_ERROR([libelf not installed])
    ])
  ])

  AC_CHECK_FUNCS([dlmopen])
  AC_CHECK_DECLS([RTLD_DEEPBIND],,, [[#include <dlfcn.h>]])

  AC_ARG_WITH(sysroot, [AS_HELP_STRING([--with-sysroot=dir],
    [sysroot directory. default: ${prefix}/${ARCH_DIR}])],
    sysroot="$withval", [
      if test "x$prefix" = xNONE; then
        sysroot=${ac_default_prefix}/${ARCH_DIR}
      else
        sysroot=${prefix}/${ARCH_DIR}
      fi
    ]
  )
  AC_SUBST(sysroot)
else
  USE64=0
  AC_MSG_WARN([not building 64bit components])
fi
AC_SUBST(USE64)

AC_ARG_ENABLE(32bit,
  AS_HELP_STRING([--disable-32bit], [do not build 32bit components]))
if test "$enable_32bit" != "no"; then
  AC_ARG_WITH(sysroot32, [AS_HELP_STRING([--with-sysroot32=dir],
    [sysroot32 directory. default: ${prefix}/${ARCH_DIR32}])],
    sysroot32="$withval", [
      if test "x$prefix" = xNONE; then
        sysroot32=${ac_default_prefix}/${ARCH_DIR32}
      else
        sysroot32=${prefix}/${ARCH_DIR32}
      fi
    ]
  )
  AC_SUBST(sysroot32)
else
  USE32=0
  AC_MSG_WARN([not building 32bit components])
fi
AC_SUBST(USE32)

AC_ARG_ENABLE(ncurses,
  AS_HELP_STRING([--disable-ncurses], [do not build ncurses]))
if test "$enable_ncurses" != "no"; then
  AC_PATH_PROGS([TIC], [tic])
  if test -z "$TIC"; then
    AC_MSG_ERROR(Install ncurses (or ncurses-bin) and retry.)
  fi
  NCURSES=1
fi
AC_SUBST(NCURSES)

PKG_CHECK_MODULES([TG], [thunk_gen >= 1.5],, [
  AC_MSG_ERROR([thunk-gen >= 1.5 not installed])
])

CFLAGS_COMM=" \
-fno-common \
-nostdinc \
-ggdb3 \
-O2 \
-ffreestanding \
-fno-stack-protector \
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

DJ64_CFLAGS="$CFLAGS $CFLAGS_COMM -fpic"
DJ32_CFLAGS="$CFLAGS_COMM -fno-pic -march=i386 -fcf-protection=none"

DJ64_LDFLAGS="$LDFLAGS"

AC_ARG_ENABLE(lto, AS_HELP_STRING([--disable-lto], [disable LTO]))
if test "$enable_lto" != "no" ; then
  DJ64_CFLAGS="$DJ64_CFLAGS -flto=auto -ffat-lto-objects"
  DJ64_LDFLAGS="$DJ64_LDFLAGS -flto=auto -O2"
fi

machine=`$CC -dumpmachine | cut -d- -f1 | sed 's/i.86/i386/'`
CONFIG_HOST=`uname -s`
AC_SUBST(CONFIG_HOST)

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

  # for LTO we need llvm-ar when clang is used
  AC_CHECK_TOOL([AR], [llvm-ar], [:])
  if test -z "$AR" -o "$AR" = ":"; then
    AC_MSG_ERROR(You don't have llvm-ar installed)
  fi
else
  AC_MSG_RESULT([no])
  CC_IS_CLANG=0
  DJ64_CFLAGS="$DJ64_CFLAGS \
    -Wno-error=maybe-uninitialized"

  AC_CHECK_TOOL([AR], [ar], [:])
  if test -z "$AR" -o "$AR" = ":"; then
    AC_MSG_ERROR(You don't have ar installed)
  fi
fi
AC_SUBST(CC_IS_CLANG)
AC_SUBST(DJ64_CFLAGS)
AC_SUBST(DJ32_CFLAGS)
AC_SUBST(DJ64_LDFLAGS)

DJ64_EXTRA_CPPFLAGS=""
DJ64_EXTRA_LDFLAGS=""
DJ64_S_EXTRA_LDFLAGS=""
if test "$CONFIG_HOST" = "Darwin"; then
  DJ64_EXTRA_CPPFLAGS="-U__APPLE__"
else
  DJ64_EXTRA_LDFLAGS="-Wl,-Bsymbolic -Wl,--no-undefined -Wl,--as-needed"
  DJ64_S_EXTRA_LDFLAGS="-Wl,-Bsymbolic"
fi
AC_SUBST(DJ64_EXTRA_CPPFLAGS)
AC_SUBST(DJ64_EXTRA_LDFLAGS)
AC_SUBST(DJ64_S_EXTRA_LDFLAGS)

if test "$enable_32bit" != "no"; then
  CROSS_GCC_CFLAGS="-Wno-error=maybe-uninitialized"
  if test -n "$CROSS_PREFIX"; then
    AC_PATH_PROG([CROSS_CC], [${CROSS_PREFIX}gcc])
    if test -z "$CROSS_CC"; then
      AC_MSG_ERROR([${CROSS_PREFIX}gcc not found, CROSS_PREFIX is wrong])
    fi
  else
    AC_PATH_PROGS([CROSS_CC], [i686-linux-gnu-gcc i686-unknown-linux-gnu-gcc i586-suse-linux-gcc i386-elf-gcc])
  fi
  if test -z "$CROSS_CC"; then
    AC_PATH_PROG([CROSS_CC], [x86_64-linux-gnu-gcc])
    if test -z "$CROSS_CC"; then
      if test \( "$machine" = "i386" -o "$machine" = "x86_64" \) -a "$CONFIG_HOST" != "Darwin"; then
        AC_MSG_NOTICE([Using host ${CC} for cross-compiling...])
        CROSS_CC=${CC}
        CROSS_CFLAGS="-m32"
      else
        AC_PATH_PROG([CROSS_CC], [clang])
        if test -n "$CROSS_CC"; then
          AC_MSG_NOTICE([Using clang as cross-cc...])
          CROSS_CFLAGS="-target i686-unknown-linux-gnu"
          CROSS_GCC_CFLAGS=""
        else
          AC_MSG_ERROR([386 cross-compiler not found, please install clang or gcc-i686-linux-gnu])
        fi
      fi
    else
      CROSS_CFLAGS="-m32"
    fi
  fi
  CROSS_CFLAGS="$CROSS_CFLAGS $CROSS_GCC_CFLAGS"
  AC_SUBST(CROSS_CFLAGS)
fi

if test -n "$CROSS_PREFIX"; then
  AC_PATH_PROG([CROSS_AS], [${CROSS_PREFIX}as])
  if test -z "$CROSS_AS"; then
    AC_MSG_ERROR([${CROSS_PREFIX}as not found, CROSS_PREFIX is wrong])
  fi
else
  AC_PATH_PROGS([CROSS_AS], [i686-linux-gnu-as i686-unknown-linux-gnu-as i586-suse-linux-as i386-elf-as])
fi
if test -z "$CROSS_AS"; then
  AC_PATH_PROG([CROSS_AS], [x86_64-linux-gnu-as])
  if test -z "$CROSS_AS"; then
    if test \( "$machine" = "i386" -o "$machine" = "x86_64" \) -a "$CONFIG_HOST" != "Darwin"; then
      AC_PATH_PROG([CROSS_AS], [as])
      if test -z "$CROSS_AS"; then
        AC_MSG_NOTICE([Using ${CC} as as...])
        CROSS_AS=${CC}
        CROSS_ASFLAGS="-m32 -x assembler -c"
      else
        CROSS_ASFLAGS="--32"
      fi
    else
      AC_PATH_PROG([CROSS_AS], [clang])
      if test -n "$CROSS_AS"; then
        AC_MSG_NOTICE([Using clang as as...])
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
  AC_PATH_PROGS([CROSS_LD], [i686-linux-gnu-ld i686-unknown-linux-gnu-ld i586-suse-linux-ld i386-elf-ld x86_64-linux-gnu-ld gld ld.lld])
fi
if test -z "$CROSS_LD"; then
  if test \( "$machine" = "i386" -o "$machine" = "x86_64" \) -a "$CONFIG_HOST" != "Darwin"; then
    AC_PATH_PROGS([CROSS_LD], [ld])
  fi
fi
if test -z "$CROSS_LD"; then
  if test "$CONFIG_HOST" = "Darwin"; then
    AC_MSG_ERROR([Please install lld or i386-elf-binutils])
  fi
  AC_MSG_ERROR([cross-ld not found])
fi

AC_MSG_CHECKING([ld supports --image-base])
if ! $CROSS_LD --image-base=0x0 -v >/dev/null 2>&1; then
  AC_MSG_RESULT([no, using -Ttext-segment])
  XLD_IMB_OPT=-Ttext-segment
  XLD_IMB_OPT_ALIGN=
else
  AC_MSG_RESULT([yes])
  XLD_IMB_OPT=--image-base
  AC_MSG_CHECKING([ld supports -Ttext-segment])
  if ! $CROSS_LD -Ttext-segment=0 -v >/dev/null 2>&1; then
    AC_MSG_RESULT([no])
  else
    AC_MSG_RESULT([yes])
    XLD_IMB_OPT_ALIGN=-Ttext-segment=0
  fi
fi
AC_SUBST(XLD_IMB_OPT)
AC_SUBST(XLD_IMB_OPT_ALIGN)

if test -n "$CROSS_PREFIX"; then
  AC_PATH_PROG([CROSS_AR], [${CROSS_PREFIX}ar])
  if test -z "$CROSS_AR"; then
    AC_MSG_ERROR([${CROSS_PREFIX}ar not found, CROSS_PREFIX is wrong])
  fi
else
  AC_PATH_PROGS([CROSS_AR], [i686-linux-gnu-ar i686-unknown-linux-gnu-ar i586-suse-linux-ar i386-elf-ar x86_64-linux-gnu-ar llvm-ar], [], [$PATH${PATH_SEPARATOR}/usr/local/opt/llvm/bin])
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
  AC_PATH_PROG([CROSS_NM], [${CROSS_PREFIX}nm])
  if test -z "$CROSS_NM"; then
    AC_MSG_ERROR([${CROSS_PREFIX}nm not found, CROSS_PREFIX is wrong])
  fi
else
  AC_PATH_PROGS([CROSS_NM], [i686-linux-gnu-nm i686-unknown-linux-gnu-nm i586-suse-linux-nm i386-elf-nm x86_64-linux-gnu-nm llvm-nm], [], [$PATH${PATH_SEPARATOR}/usr/local/opt/llvm/bin])
fi
if test -z "$CROSS_NM"; then
  if test "$machine" = "i386" -o "$machine" = "x86_64"; then
    AC_PATH_PROGS([CROSS_NM], [gnm nm])
  fi
fi
if test -z "$CROSS_NM"; then
  AC_MSG_ERROR([cross-nm not found])
fi

if test -n "$CROSS_PREFIX"; then
  AC_PATH_PROG([CROSS_STRIP], [${CROSS_PREFIX}strip])
  if test -z "$CROSS_STRIP"; then
    AC_MSG_ERROR([${CROSS_PREFIX}strip not found, CROSS_PREFIX is wrong])
  fi
else
  AC_PATH_PROGS([CROSS_STRIP], [i686-linux-gnu-strip i686-unknown-linux-gnu-strip i586-suse-linux-strip i386-elf-strip x86_64-linux-gnu-strip llvm-strip], [], [$PATH${PATH_SEPARATOR}/usr/local/opt/llvm/bin])
fi
if test -z "$CROSS_STRIP"; then
  if test "$machine" = "i386" -o "$machine" = "x86_64"; then
    AC_PATH_PROGS([CROSS_STRIP], [strip])
  fi
fi
if test -z "$CROSS_STRIP"; then
  AC_MSG_ERROR([cross-strip not found])
fi

if test -n "$CROSS_PREFIX"; then
  AC_PATH_PROG([XOBJCOPY], [${CROSS_PREFIX}objcopy])
  if test -z "$XOBJCOPY"; then
    AC_MSG_ERROR([${CROSS_PREFIX}objcopy not found, CROSS_PREFIX is wrong])
  fi
else
  AC_PATH_PROGS([XOBJCOPY], [i686-linux-gnu-objcopy i686-unknown-linux-gnu-objcopy i586-suse-linux-objcopy x86_64-linux-gnu-objcopy llvm-objcopy objcopy])
fi

if test "$enable_64bit" != "no" -o "$enable_32bit" != "no"; then
  AC_CHECK_PROGS([UCTAGS], [uctags ctags])
  AC_CHECK_PROGS([URTAGS], [ureadtags readtags])
  if test -z "$UCTAGS" -o -z "$URTAGS"; then
    AC_MSG_ERROR([universal-ctags not installed])
  fi
  if ! $UCTAGS --version | grep "Universal Ctags" >/dev/null; then
    AC_MSG_ERROR([bsd ctags do not suit, install universal-ctags])
  fi

  AC_CHECK_TYPES([__float80])
fi

if ! test -f Makefile.conf.in; then
  AC_MSG_NOTICE([Populating makefiles...])
  wd=`pwd`
  abssrcdir=`cd $srcdir && pwd`
  if test "${wd#$abssrcdir}" != "$wd" ; then
    builddir=".${wd#$abssrcdir}"
  else
    builddir="$wd"
  fi
  mkdir -p `(cd $abssrcdir; find src demos -type d -print)`
  $LN_S -f $abssrcdir/Makefile.conf.in Makefile.conf.in
  for i in `(cd $abssrcdir; find . \( -path './demos/*' -o -path './src/*' \
      -o -path ./makefile \) -a -name 'makefile' -print
    )`; do
    echo  $LN_S -f $abssrcdir/${i#./} $i
    $LN_S -f $abssrcdir/${i#./} $i
  done
fi

AC_OUTPUT
