ARCH=`head -n 1 $1 | grep .arch | cut -d " " -f 2`
if [ -z "$ARCH" ]; then
  cat $1 | grep -v .addrsig
  exit 0
fi
cat $1 | grep -v .arch | grep -v .addrsig | sed 's/.align/.p2align/g'
