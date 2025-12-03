gen_incs() {
    cd "$1" && find . -name "*.h" -exec grep -E " ASM\(| ASM_P\(| ASM_AP\(| ASM_N\(| ASM_F\(" \
	{} + | grep :EXTERN | \
	cut -d ":" -f 1 | uniq | sed -E 's/\.\/(.+)/#include "\1"/'
}

gen_incsn() {
    cd "$1" && find . -name "*.h" -exec grep " ASM_N(" {} + | grep :EXTERN | cut -d ":" -f 1 | uniq | \
	sed -E 's/\.\/(.+)/#include "\1"/'
}

gen_asyms() {
    find "$1" -name "*.h" -exec grep -E " ASM_ARR\(" {} + | grep :EXTERN | \
	sed -E 's/.+\((.+), (.+), (.+)\);.*/__ASM_ARR\(\1, \2, \3\) SEMIC/' | sort | uniq
    find "$1" -name "*.h" -exec grep -E " ASM_F\(" {} + | grep :EXTERN | \
	sed -E 's/.+EXTERN ASM.*\((.+)\);.*/__ASM_FUNC\(\1) SEMIC/' | sort | uniq
    find "$1" -name "*.h" -exec grep -E " ASM_P\(" {} + | grep :EXTERN | \
	sed -E 's/.+\((.+), (.+)\);.*/__ASM_PTR\(\1, \2\) SEMIC/' | sort | uniq
    find "$1" -name "*.h" -exec grep -E " ASM_N\(" {} + | grep :EXTERN | \
	sed -E 's/.+\((.+), (.+)\);.*/__ASM_N\(\1, \2\) SEMIC/' | sort | uniq
    find "$1" -name "*.h" -exec grep -E " ASM\(" {} + | grep :EXTERN | \
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
