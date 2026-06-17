/* Copyright (C) 2013 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details */
#include <malloc.h>
#include <stdint.h>
#include <libc/malloc.h>

/* Make VAL a multiple of ALIGN.  */
static inline
size_t
align_val(size_t val, size_t align)
{
  return ((val + (align - 1)) & ~(align - 1));
}

/* Make PTR a multiple of ALIGN.  */
static inline
char *
align_ptr(char *ptr, size_t align)
{
  return (char *)(((uintptr_t)ptr + (align - 1)) & ~(align - 1));
}

/* Take part of one chunk of memory and merge it with a preceding chunk.  */
static void *
split_small_alloc(char *ptr, size_t split_pos)
{
  BLOCK *b1, *b2;
  size_t b1_size;

  b1 = (BLOCK *)(ptr - 4);
  b1_size = b1->size;

  b2 = BEFORE(b1);
  b2->size += split_pos;

  b1 = (BLOCK *)((char *)b1 + split_pos);
  b1->size = b1_size - split_pos;
  ENDSZ(b1) = b1->size;

  ENDSZ(b2) = b2->size;

  return DATA(b1);
}

/* Split a chunk of allocated memory into two chunks so both can
   be released with a call to free().  */
static void *
split_alloc(char *ptr, size_t split_pos)
{
  BLOCK *b1, *b2;
  char *split_ptr;

  b1 = (BLOCK *)(ptr - 4);

  /* Set location of second pointer and its block info.  */
  split_ptr = ptr + split_pos;
  b2 = (BLOCK *)(split_ptr - 4);

  /* Set up the two blocks.  */
  b2->size = (b1->size - split_pos) | 1;
  b1->size = (split_pos - 8) | 1;

  ENDSZ(b1) = b1->size;
  ENDSZ(b2) = b2->size;

  return DATA(b2);
}

/* Return a block of memory AMT bytes long whose address is a multiple
   ALIGN.  ALIGN must be a power of 2.  */
void *
memalign(size_t align, size_t amt)
{
  char *ptr, *aligned_ptr;
  size_t alloc_size, before_size, after_size;

  if (align != align_val(align, align))
    return NULL;

  if (align < ALIGN)
    align = ALIGN;

  if (align == ALIGN)
    return malloc(amt);

  amt = align_val(amt, ALIGN);
  alloc_size = amt + align;

  ptr = malloc(alloc_size);
  if (ptr == NULL)
    return ptr;

  aligned_ptr = align_ptr(ptr, align);

  /* Amount of space between the malloc'ed pointer
     and the aligned pointer.  */
  before_size = (aligned_ptr - ptr);

  if (before_size > 8)
  {
    aligned_ptr = split_alloc(ptr, before_size);
    free (ptr);
  }
  else if (before_size)
  {
    aligned_ptr = split_small_alloc (ptr, before_size);
  }

  /* Release extra space after the aligned block. Avoid
     creating an empty block.  */
  after_size = alloc_size - before_size - amt;
  if (after_size >= 16)
  {
    char *after;
    after = split_alloc(aligned_ptr, amt + 8);
    free (after);
  }

  return aligned_ptr;
}
