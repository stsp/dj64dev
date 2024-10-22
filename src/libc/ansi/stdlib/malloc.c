#define sbrk(x) djsbrk(x)
#include "nmalloc.inc"

void *malloc(size_t size)
{
    return nmalloc(size);
}

void free(void *ptr)
{
    return nfree(ptr);
}

void *realloc(void *ptr, size_t size)
{
    return nrealloc(ptr, size);
}

void *calloc(size_t n, size_t s)
{
    return ncalloc(n, s);
}
