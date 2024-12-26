#!/usr/bin/env bash

set -o pipefail

if [ $# -lt 5 ]; then
    echo "generate thunk prototypes compatible with fdpp's thunk_gen & plt"
    echo "$0 <lib_path> <hdr_path> <athunks_out> <cthunks_out> <incs_out>"
    exit 1
fi

if test -z "$CTAGS" -o test -z "$RTAGS"; then
    echo "universal-ctags not installed"
    exit 1
fi

extr_proto() {
    $RTAGS -e -t "$1" "$2" | \
	expand | \
	sed -E \
	    -e 's/ *__attribute__\(\(__.*__\)\)//' \
	    -e 's/ *__attribute__\(\(.*\)\)//' \
	    -e 's/.*\/\^(.+\);).*/\1/' \
	    -e 's/__P\((.*)\);/\1;/' \
	    -e "s/([^ (]+) +(\*)*([^ (]+)( *\([^(]+.*;)/\1\2 $3 \3\4/" \
	       | \
	tr -s '[:blank:]'
}

list_syms() {
    LC_ALL=C nm -A "$1" | grep " $2 " | tr -s '[:blank:]' | cut -d " " -f 3 | \
	sed 's/^_//'
}

list_syms2() {
    LC_ALL=C nm -A "$1" | grep -E " $2 "\|" $3 " | tr -s '[:blank:]' | cut -d " " -f 3 | \
	sed 's/^_//'
}

TF=/tmp/tagsxx
TL=/tmp/a.so
set -e

${CROSS_LD} -melf_i386 -shared -Bsymbolic -z notext -o $TL --whole-archive "$1"
shift
PRUNES="-name libm -prune -o -name machine -prune"
PRUNES="$PRUNES -o -name string.h -prune -o -name in.h -prune"
PRUNES="$PRUNES -o -name ctype.h -prune"
# bad/temporary prunes below
PRUNES="$PRUNES -o -name setjmp.h -prune"
find $1 $PRUNES -o -print | \
	$CTAGS -L - --kinds-C=p --pattern-length-limit=0 -I _V_BW+,_V_FW+ -f $TF
shift
# https://stackoverflow.com/questions/11003418/calling-shell-functions-with-xargs
export -f extr_proto
export RTAGS
list_syms $TL T | xargs -I '{}' bash -c "extr_proto $TF '{}' ASMFUNC" | nl -n ln -v 0 >$1
list_syms $TL U | xargs -I '{}' bash -c "extr_proto $TF '{}' ASMCFUNC" | nl -n ln -v 0 >$2
echo "#define THUNK_INCS 1" >$3
list_syms2 $TL U T | xargs -L 1 $RTAGS -t $TF | expand | tr -s '[:blank:]' | \
	cut -d " " -f 2 | sort | uniq | \
	sed -E 's/.*(include)\/(.*)/#\1 "\2"/' >>$3
shift 3

rm $TF
rm $TL
