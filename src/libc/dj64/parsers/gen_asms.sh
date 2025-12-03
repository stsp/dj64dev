gen_incs() {
    cd "$1" && grep -E -r --include="*.h" " ASM\(| ASM_P\(| ASM_AP\(| ASM_N\(| ASM_F\(" . | grep :EXTERN | \
	cut -d ":" -f 1 | uniq | sed -E 's/\.\/(.+)/#include "\1"/'
}

gen_incsn() {
    cd "$1" && grep -r --include="*.h" " ASM_N(" . | grep :EXTERN | cut -d ":" -f 1 | uniq | \
	sed -E 's/\.\/(.+)/#include "\1"/'
}

gen_asyms() {
    grep -E -r --include="*.h" " ASM_ARR\(" . "$1" | grep :EXTERN | \
	sed -E 's/.+\((.+), (.+), (.+)\);.*/__ASM_ARR\(\1, \2, \3\) SEMIC/' | sort | uniq
    grep -E -r --include="*.h" " ASM_F\(" . "$1" | grep :EXTERN | \
	sed -E 's/.+EXTERN ASM.*\((.+)\);.*/__ASM_FUNC\(\1) SEMIC/' | sort | uniq
    grep -E -r --include="*.h" " ASM_P\(" . "$1" | grep :EXTERN | \
	sed -E 's/.+\((.+), (.+)\);.*/__ASM_PTR\(\1, \2\) SEMIC/' | sort | uniq
    grep -E -r --include="*.h" " ASM_N\(" . "$1" | grep :EXTERN | \
	sed -E 's/.+\((.+), (.+)\);.*/__ASM_N\(\1, \2\) SEMIC/' | sort | uniq
    grep -E -r --include="*.h" " ASM\(" . "$1" | grep :EXTERN | \
	sed -E 's/.+\((.+), (.+)\);.*/__ASM\(\1, \2\) SEMIC/' | sort | uniq
}

case "$1" in
    0) gen_incs "$2"
    ;;
    1) gen_incsn "$2"
    ;;
    2) gen_asyms "$2"
    ;;
esac
