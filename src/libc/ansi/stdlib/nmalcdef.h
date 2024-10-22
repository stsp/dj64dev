/* Copyright (c) 2003, 2007 by Charles B. Falconer
   Licensed under the terms of the GNU LIBRARY GENERAL PUBLIC
   LICENSE and/or the terms of COPYING.DJ, all available at
   <http://www.delorie.com>.

   Bug reports to <mailto:djgpp@delorie.com>

   Revised 2007-01-04 to include calloc.
   Revised 2007-01-28 per bug report by Florian Xaver. *f1*
   Revised 2014, 2015 for use in DJGPP libc by Andris Pavenis <andris.pavenis@iki.fi>

   This file contains definitions common for nmalloc.c and nmalign.c
*/

#ifndef NMALLOC_DEF_H
#define NMALLOC_DEF_H

#define MEMALIGN

#ifndef NEWMALLDBG
#  undef  _NDEBUG
#  define _NDEBUG
#else
#  undef  _NDEBUG
#endif

#ifndef _NDEBUG
   /* diddle these to control areas debugged */
#  define DEBUGM 1     /* malloc */
#  define DEBUGF 1     /* free */
#  define DEBUGR 1     /* realloc */
#else
#  define DEBUGM 0
#  define DEBUGF 0
#  define DEBUGR 0
   /* the HOOKABLE variant is only for development   */
   /* It allows some other package to define malloc, */
   /* free, realloc, and to call this package.       */
#if 0
#  ifndef HOOKABLE
#     define nmalloc  malloc
#     define nfree    free
#     define nrealloc realloc
#     define nmemalign memalign
#     define ncalloc  calloc
#  else
#     define nmalloc  _malloc
#     define nfree    _free
#     define nrealloc _realloc
#     define nmemalign _memalign
#     define ncalloc   calloc  /* can't hook this */
#  endif
#endif
#  define fakesbrk sbrk
#endif

#define SAVEMEMORY 1  /* 0/1 to use/eliminate extra storage */

typedef unsigned char byte;
typedef unsigned int  ulong;

#include <stddef.h> /* offsetof() */
#include <stdlib.h> /* malloc, free, realloc, exit, EXIT_FAILURE */
#include <unistd.h> /* sbrk, write */
#include <signal.h> /* raise, SIGABRT */
#include <string.h> /* strlen, memmove, memcpy, memset */
#include <limits.h> /* CHAR_BIT, INT_MAX */
#include <malloc.h>
#include <libc/sysquery.h> /* available debugger linkages */

/* system dependant magic.  Only STDIN, STDERR used */
enum {STDIN = 0, STDOUT, STDERR, STDAUX, STDPRN}; /* handles */

/* Incorporation into the system should require deleting the
   following <nmalloc.h>, changing all references to nmalloc
   to malloc, nfree to free, nrealloc to realloc.  Change the
   single call to fakesbrk to sbrk.  Also normally set all
   DEBUGx values above to 0 in place of 1.  Later many
   routines will be made inline.  For debugging compilations
   are done with "/Dinline= " switch.  For production use
   the "/D_NDEBUG=1" switch, which does all the above except
   the inlining. But see NEWMALLDBG above.
*/
#ifndef _NDEBUG
#  include "nmalloc.h" /* while testing before name changes */
#else
#  ifdef HOOKABLE
#     include "hookmem.h"
#  endif
#endif

/* ============================================================
   Macros and storage for debugging, works before init on DJGPP
   WARNING - GNU extensions used here!!
   Note: many messages are designed for easy search with grep
         and also serve as comments.
*/
#if DEBUGM || DEBUGF || DEBUGR
#  include <stdio.h>     /* sprintf, for DEBUG only */
#  include "fakesbrk.h"  /* repeatable sbrk */
#  define EOL "\n"       /* for DEBUG printouts only, allow crlf */
   static char  dbgbuff[1024];
   static char *dbgbp = dbgbuff;
#  define DBGFLUSH do {if (dbgbp != dbgbuff) { \
                         /* write it out */ \
                         write(STDOUT, dbgbuff, strlen(dbgbuff)); \
                         dbgbp = dbgbuff; \
                      } \
                   } while (0)
#  define DBGEOLN do { \
                     DBGPRT(EOL); \
                     DBGFLUSH; \
                  } while (0)
#  define DBGPRT(msg, args...) do { \
                    if ((dbgbp - dbgbuff) > 924) DBGFLUSH; \
                    dbgbp +=sprintf(dbgbp, msg , ## args); \
                 } while (0)
#  define SHOWBLK(m, id) showblock(m, id)
#  if DEBUGM
#     define DBGPRTM(msg, args...) \
                     dbgbp +=sprintf(dbgbp, msg , ## args)
#     define SHOWBLKM(m, id) showblock(m, id)
#  else
#     define DBGPRTM(msg, args...)
#     define SHOWBLKM(m, id)
#  endif
#  if DEBUGF
#     define DBGPRTF(msg, args...) \
                     dbgbp +=sprintf(dbgbp, msg , ## args)
#     define SHOWBLKF(m, id) showblock(m, id)
#  else
#     define DBGPRTF(msg, args...)
#     define SHOWBLKF(m, id)
#  endif
#  if DEBUGR
#     define DBGPRTR(msg, args...) \
                     dbgbp +=sprintf(dbgbp, msg , ## args)
#     define SHOWBLKR(m, id) showblock(m, id)
#  else
#     define DBGPRTR(msg, args...)
#     define SHOWBLKR(m, id)
#  endif
#else
#  define DBGFLUSH
#  define DBGEOLN
#  define DBGPRT(msg, args...)
#  define SHOWBLK(m, id)
#  define DBGPRTM(msg, args...)
#  define SHOWBLKM(m, id)
#  define DBGPRTF(msg, args...)
#  define SHOWBLKF(m, id)
#  define DBGPRTR(msg, args...)
#  define SHOWBLKR(m, id)
#endif

/* This is intended to allow finding the header area from
   the address of the immediately adjacent memblocks.
   The guardxx avoid destruction by an off by one pointer
   and serve no real logical purpose.  Note that in some
   cases sz may be recovered from next or next may be
   recovered from sz.
*/
typedef struct memblock {
   struct memblock *prev;   /* 1st, protect against overrun */
   struct memblock *next;   /* makes this less clobberable  */
   ulong            sz;                 /* of this memblock */
   /* An allocated block has the next two (1) fields NULL */
   /* A free block has them both non-NULL  */
   struct memblock *nextfree;
   struct memblock *prevfree; /* actually data w/SAVEMEMORY */
#if SAVEMEMORY == 0
   ulong            guardlo;     /* may hold size requested */
#endif
   /* here lies the actual assigned storage                 */
   /* so the following must be addressed by adding offset   */
   /* storage must always be a multiple of 8 in size        */
   /* thus these items are fictional, i.e. for zero data    */
} memblock, *memblockp;

/* Notice that with SAVEMEMORY the prevfree field only
   exists for free blocks; it reuses data space.  This
   is why we cannot allow 0 sized blocks.
*/
#if SAVEMEMORY
#  define DATAOFFSET (offsetof(memblock, prevfree))
#else
#  define DATAOFFSET sizeof(memblock)
#endif

/* conversion and access macros */
#define MEMBLKp(p) (memblockp)((byte*)(p) - DATAOFFSET)
#define PTR(m)     (void*)((byte*)(m) + DATAOFFSET)
#define ALIGN 16

#define ALIGNMASK (ALIGN-1)

/* We can never use an allocation smaller than this */
#define MINSAVE   (ALIGN + DATAOFFSET)

/* Alternate form of NULL to distinguish free lists
   This is self protection, because freehdrs[1] is otherwise
   unused.  freehdrs[0] is reserved to hold lastsbrk.  In turn
   this means that ALIGN must be >= 4.
*/
#define NONE (memblockp)&freehdrs[1]

/* Magic constants.  MINSBRK must be MINSAVE or larger */
enum {NFLISTS = (int)(CHAR_BIT * sizeof(size_t)), MINSBRK = 1024};

static __inline ulong roundup(size_t sz)
{
   ulong size;

   size = ((sz + ALIGNMASK) & ~ALIGNMASK) + DATAOFFSET;
   return size;
} /* roundup */

//memblockp __nmalloc_split(memblockp *mp, ulong sz);

#endif /* NMALLOC_DEF_H */
