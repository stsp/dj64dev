# silly rip-off from thunk_gen project

m4_define([sp_num], [[#]$1
])
# bison doesn't like m4_ macros (warns), so alias it to nl
m4_define([nl], [m4_newline($@)])

# foru - upcounting loop.
m4_define(foru, [m4_cond(m4_eval($3>=$2), 1, [m4_for([$1],[$2],[$3],,[$4])])])

# generate initializers
m4_define(G_I0, [foru(i, 1, $1, [a[]i[]m4_if($1, i,, [, ])])])
m4_define(G_I1, [foru(i, 1, $1, [a[]m4_eval($1-i+1)[]m4_if($1, i,, [, ])])])

m4_define(THUNK,
[[#]define _THUNK$1$4([n, r], m4_cond($2, 0, [s, ])f, foru(i, 1, $1,
  [t[]i, q[]i, at[]i, aat[]i, c[]i, l[]i, ])z) \
r _CFUNC(f)(m4_if($1, 0, void, [foru(i, 1, $1,
[t[]i a[]i q[]i[]m4_if($1, i,, [, ])])])) \
{ \
    dnl
m4_cond($2, 0, [dnl
return dnl
])dnl
_FUNC(f)dnl
m4_if($1, 0,[dnl
()dnl
], [dnl
( G_I$3($1) )dnl
])dnl
; \
}dnl
])

# now enable output. Should do it last to not get newlines from
# defines or comments.
m4_define([_m4_divert(STDOUT)], 1)
m4_divert_push([STDOUT])dnl
