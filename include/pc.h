/* Copyright (C) 2012 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1999 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#ifndef __dj_include_pc_h_
#define __dj_include_pc_h_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifndef __dj_ENFORCE_ANSI_FREESTANDING

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) \
  || !defined(__STRICT_ANSI__) || defined(__cplusplus)

#endif /* (__STDC_VERSION__ >= 199901L) || !__STRICT_ANSI__ */

#ifndef __STRICT_ANSI__

#ifndef _POSIX_SOURCE

unsigned char	inportb  (unsigned short _port);
unsigned short	inportw  (unsigned short _port);
uint32_t	inportl  (unsigned short _port);
void		inportsb (unsigned short _port, unsigned char  *_buf, unsigned _len);
void		inportsw (unsigned short _port, unsigned short *_buf, unsigned _len);
void		inportsl (unsigned short _port, uint32_t      *_buf, unsigned _len);
void		outportb (unsigned short _port, unsigned char  _data);
void		outportw (unsigned short _port, unsigned short _data);
void		outportl (unsigned short _port, uint32_t _data);
void		outportsb(unsigned short _port, const unsigned char  *_buf, unsigned _len);
void		outportsw(unsigned short _port, const unsigned short *_buf, unsigned _len);
void		outportsl(unsigned short _port, const uint32_t *_buf, unsigned _len);

unsigned char	inp(unsigned short _port);
unsigned short	inpw(unsigned short _port);
void		outp(unsigned short _port, unsigned char _data);
void		outpw(unsigned short _port, unsigned short _data);
#ifndef kbhit
int		kbhit(void);
#endif
int		getkey(void);	/* ALT's have 0x100 set */
int		getxkey(void);	/* ALT's have 0x100 set, 0xe0 sets 0x200 */
const unsigned char * __get_extended_key_string(int _xkey_code);
void		sound(int _frequency);
#define		nosound() sound(0)


extern unsigned char ScreenAttrib;

#define ScreenPrimary _go32_info_block.linear_address_of_primary_screen
#define ScreenSecondary _go32_info_block.linear_address_of_secondary_screen

int	ScreenMode(void);
int	ScreenRows(void);
int	ScreenCols(void);
void	ScreenPutChar(int _ch, int _attr, int _x, int _y);
void	ScreenGetChar(int *_ch, int *_attr, int _x, int _y);
void	ScreenPutString(const char *_ch, int _attr, int _x, int _y);
void	ScreenSetCursor(int  _row, int  _col);
void	ScreenGetCursor(int *_row, int *_col);
void	ScreenClear(void);
void	ScreenUpdate(const void *_virtual_screen);
void	ScreenUpdateLine(const void *_virtual_screen_line, int _row);
void	ScreenRetrieve(void *_virtual_screen);
void	ScreenVisualBell(void);

#endif /* !_POSIX_SOURCE */
#endif /* !__STRICT_ANSI__ */
#endif /* !__dj_ENFORCE_ANSI_FREESTANDING */

#ifndef __dj_ENFORCE_FUNCTION_CALLS

//#include <inlines/pc.h>

#endif /* !__dj_ENFORCE_FUNCTION_CALLS */

#ifdef __cplusplus
}
#endif

#endif /* !__dj_include_pc_h_ */
