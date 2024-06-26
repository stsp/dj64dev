/* Copyright (C) 2012 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2000 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#ifndef __dj_include_go32_h_
#define __dj_include_go32_h_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __dj_ENFORCE_ANSI_FREESTANDING

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) \
  || !defined(__STRICT_ANSI__) || defined(__cplusplus)

#endif /* (__STDC_VERSION__ >= 199901L) || !__STRICT_ANSI__ */

#ifndef __STRICT_ANSI__

#ifndef _POSIX_SOURCE

#include <sys/version.h>
#include <sys/djtypes.h>
#include <libc/asmobj.h>
#include <sys/cdefs.h>

#ifndef _SIZE_T
__DJ_size_t
#define _SIZE_T
#endif

typedef struct {
  ULONG32  size_of_this_structure_in_bytes;
  ULONG32  linear_address_of_primary_screen;
  ULONG32  linear_address_of_secondary_screen;
  ULONG32  linear_address_of_transfer_buffer;
  ULONG32  size_of_transfer_buffer; /* >= 4k */
  ULONG32  pid;
  unsigned char  master_interrupt_controller_base;
  unsigned char  slave_interrupt_controller_base;
  unsigned short selector_for_linear_memory;
  ULONG32  linear_address_of_stub_info_structure;
  ULONG32  linear_address_of_original_psp;
  unsigned short run_mode;
  unsigned short run_mode_info;
} __Go32_Info_Block;

EXTERN ASM_N(__Go32_Info_Block, _go32_info_block);

#define _GO32_RUN_MODE_UNDEF	0
#define _GO32_RUN_MODE_RAW	1
#define _GO32_RUN_MODE_XMS	2
#define _GO32_RUN_MODE_VCPI	3
#define _GO32_RUN_MODE_DPMI	4

#include <sys/movedata.h>
#include <sys/segments.h>

#define _go32_my_cs _my_cs
#define _go32_my_ds _my_ds
#define _go32_my_ss _my_ss
#define _go32_conventional_mem_selector() _go32_info_block.selector_for_linear_memory
#define _dos_ds _go32_info_block.selector_for_linear_memory

#define __tb _go32_info_block.linear_address_of_transfer_buffer
#define __tb_size _go32_info_block.size_of_transfer_buffer

/* returns number of times hit since last call. (zero first time) */
unsigned _go32_was_ctrl_break_hit(void);
void     _go32_want_ctrl_break(int yes); /* auto-yes if call above function */

#endif /* !_POSIX_SOURCE */
#endif /* !__STRICT_ANSI__ */
#endif /* !__dj_ENFORCE_ANSI_FREESTANDING */

#ifndef __dj_ENFORCE_FUNCTION_CALLS
#endif /* !__dj_ENFORCE_FUNCTION_CALLS */

#ifdef __cplusplus
}
#endif

#endif /* !__dj_include_go32_h_ */
