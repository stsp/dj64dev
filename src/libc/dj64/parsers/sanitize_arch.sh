F_STRS=.arch\|.addrsig\|.fpu\|.eabi_attribute
ARCH=`head -n 1 $1 | grep .arch | cut -d " " -f 2`
if [ -z "$ARCH" ]; then
  cat $1 | grep -E -v $F_STRS
  exit 0
fi
cat $1 | grep -E -v $F_STRS | sed 's/.align/.p2align/g'
