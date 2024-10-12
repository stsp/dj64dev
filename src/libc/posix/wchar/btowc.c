#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
//#include "internal.h"
/* Arbitrary encoding for representing code units instead of characters. */
#define CODEUNIT(c) (0xdfff & (signed char)(c))
#define IS_CODEUNIT(c) ((unsigned)(c)-0xdf80 < 0x80)

wint_t btowc(int c)
{
	int b = (unsigned char)c;
	return b<128 ? b : (MB_CUR_MAX==1 && c!=EOF) ? CODEUNIT(c) : WEOF;
}
