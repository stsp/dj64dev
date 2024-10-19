#include <wctype.h>
//#include "libc.h"

int iswalnum(wint_t wc)
{
	return iswdigit(wc) || iswalpha(wc);
}
