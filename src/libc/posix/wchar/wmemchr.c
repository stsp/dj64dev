#include <wchar.h>
#include <libc/unconst.h>

wchar_t *wmemchr(const wchar_t *s, wchar_t c, size_t n)
{
	for (; n && *s != c; n--, s++);
	return n ? unconst(s, wchar_t *) : 0;
}
