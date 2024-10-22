#include <dpmi.h>
#include "nmalloc.inc"

unsigned int malloc32(size_t size)
{
    void *ptr = nmalloc(size);
    if (!ptr)
        return 0;
    return PTR_DATA(ptr);
}

void free32(unsigned int addr)
{
    nfree(DATA_PTR(addr));
}

unsigned int realloc32(unsigned int addr, size_t size)
{
    void *ptr = (addr ? DATA_PTR(addr) : NULL);
    void *ret = nrealloc(ptr, size);
    if (!ret)
        return 0;
    return PTR_DATA(ret);
}

unsigned int calloc32(size_t n, size_t s)
{
    void *ptr = ncalloc(n, s);
    if (!ptr)
        return 0;
    return PTR_DATA(ptr);
}
