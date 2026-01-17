F_STRS=\\.arch\|\\.addrsig\|\\.fpu\|\\.eabi_attribute
ARCH=`grep .arch $1 | cut -d " " -f 2`
if [ -z "$ARCH" ]; then
  grep -E -v $F_STRS $1
  exit 0
fi
grep -E -v $F_STRS $1 | sed -E 's/\.align/\.p2align/g'
