#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
//#include "internal.h"
/* Arbitrary encoding for representing code units instead of characters. */
#define CODEUNIT(c) (0xdfff & (signed char)(c))
#define IS_CODEUNIT(c) ((unsigned)(c)-0xdf80 < 0x80)

int wctob(wint_t c)
{
	if (c < 128) return c;
	if (MB_CUR_MAX==1 && IS_CODEUNIT(c)) return (unsigned char)c;
	return EOF;
}
