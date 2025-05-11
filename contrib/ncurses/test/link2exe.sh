NM8=`echo $1 | head -c 8`
djlink -d dosemu_${NM8}.exe.dbg $1 -n ${NM8}.exe -o ${NM8}.exe -f 0x80
