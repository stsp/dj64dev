/* Copyright (C) 2015 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2012 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1999 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#ifndef __dj_include_dos_h_
#define __dj_include_dos_h_

#include <sys/cdefs.h>

#ifndef __dj_ENFORCE_ANSI_FREESTANDING

#ifndef __STRICT_ANSI__

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) \
  || !defined(__STRICT_ANSI__) || defined(__cplusplus)

#endif /* (__STDC_VERSION__ >= 199901L) || !__STRICT_ANSI__ */

#ifndef _POSIX_SOURCE

#include <pc.h>

extern int _8087;

int _detect_80387(void);

struct DWORDREGS {
  ULONG32 edi;
  ULONG32 esi;
  ULONG32 ebp;
  ULONG32 cflag;
  ULONG32 ebx;
  ULONG32 edx;
  ULONG32 ecx;
  ULONG32 eax;
  unsigned short eflags;
};

struct DWORDREGS_W {
  ULONG32 di;
  ULONG32 si;
  ULONG32 bp;
  ULONG32 cflag;
  ULONG32 bx;
  ULONG32 dx;
  ULONG32 cx;
  ULONG32 ax;
  unsigned short flags;
};

struct WORDREGS {
  unsigned short di, _upper_di;
  unsigned short si, _upper_si;
  unsigned short bp, _upper_bp;
  unsigned short cflag, _upper_cflag;
  unsigned short bx, _upper_bx;
  unsigned short dx, _upper_dx;
  unsigned short cx, _upper_cx;
  unsigned short ax, _upper_ax;
  unsigned short flags;
};

struct BYTEREGS {
  unsigned short di, _upper_di;
  unsigned short si, _upper_si;
  unsigned short bp, _upper_bp;
  ULONG32 cflag;
  unsigned char bl;
  unsigned char bh;
  unsigned short _upper_bx;
  unsigned char dl;
  unsigned char dh;
  unsigned short _upper_dx;
  unsigned char cl;
  unsigned char ch;
  unsigned short _upper_cx;
  unsigned char al;
  unsigned char ah;
  unsigned short _upper_ax;
  unsigned short flags;
};

union REGS {		/* Compatible with DPMI structure, except cflag */
  struct DWORDREGS d;
#ifdef _NAIVE_DOS_REGS
  struct WORDREGS x;
#else
#ifdef _BORLAND_DOS_REGS
  struct DWORDREGS x;
#else
  struct DWORDREGS_W x;
#endif
#endif
  struct WORDREGS w;
  struct BYTEREGS h;
};

#ifdef DJ64
struct SREGS {
  char *es;
  char *ds;
  char *fs;
  char *gs;
  char *cs;
  char *ss;
};

#define P_SEG(p) p
#define P_OFF(p) 0
#define SEG_P(s) s

#else

struct SREGS {
  unsigned short es;
  unsigned short ds;
  unsigned short fs;
  unsigned short gs;
  unsigned short cs;
  unsigned short ss;
};

#define P_SEG(p) _my_ds()
#define P_OFF(p) (uintptr_t)(p)
/* trick, arg unused */
#define SEG_P(s) (void*)

#endif

struct ftime {
  unsigned ft_tsec:5;	/* 0-29, double to get real seconds */
  unsigned ft_min:6;	/* 0-59 */
  unsigned ft_hour:5;	/* 0-23 */
  unsigned ft_day:5;	/* 1-31 */
  unsigned ft_month:4;	/* 1-12 */
  unsigned ft_year:7;	/* since 1980 */
};

struct date {
  short da_year;
  char  da_day;
  char  da_mon;
};

struct time {
  unsigned char ti_min;
  unsigned char ti_hour;
  unsigned char ti_hund;
  unsigned char ti_sec;
};

struct dfree {
  unsigned df_avail;
  unsigned df_total;
  unsigned df_bsec;
  unsigned df_sclus;
};

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned short   _osmajor, _osminor;
extern unsigned short   _os_trueversion;
extern const    char  * _os_flavor;
extern int		_doserrno;

unsigned short _get_dos_version(int);

int _get_fat_size(const int _drive);
int _get_fs_type(const int _drive, char *const _result_str);
int _is_cdrom_drive(const int _drive);
int _is_fat32(const int _drive);
int _is_ram_drive(const int _drive);
int _media_type(const int _drive);

int int86(int _ivec, union REGS *in, union REGS *out);
int int86x(int _ivec, union REGS *_in, union REGS *_out, struct SREGS *_seg);
int intdos(union REGS *_in, union REGS *_out);
int intdosx(union REGS *_in, union REGS *_out, struct SREGS *_seg);
int bdos(int _func, unsigned _dx, unsigned _al);
int bdosptr(int _func, void *_dx, unsigned _al);

int _int86(int ivec, union REGS *in, union REGS *out);
int _int86x(int ivec, union REGS *in, union REGS *out, struct SREGS *sregs);

#define bdosptr(_a, _b, _c) bdos(_a, (unsigned)(_b), _c)
#define intdos(_a, _b) int86(0x21, _a, _b)
#define intdosx(_a, _b, _c) int86x(0x21, _a, _b, _c)

int enable(void);
int disable(void);

int getftime(int _handle, struct ftime *_ftimep);
int setftime(int _handle, struct ftime *_ftimep);

int getcbrk(void);
int setcbrk(int _new_value);

void getdate(struct date *);
void gettime(struct time *);
void setdate(struct date *);
void settime(struct time *);

void getdfree(unsigned char _drive, struct dfree *_ptr);

void delay(unsigned _msec);


/*
 *  For compatibility with other DOS C compilers.
 */

#define _A_NORMAL   0x00    /* Normal file - No read/write restrictions */
#define _A_RDONLY   0x01    /* Read only file */
#define _A_HIDDEN   0x02    /* Hidden file */
#define _A_SYSTEM   0x04    /* System file */
#define _A_VOLID    0x08    /* Volume ID file */
#define _A_SUBDIR   0x10    /* Subdirectory */
#define _A_ARCH     0x20    /* Archive file */

#define _enable   enable
#define _disable  disable

struct _dosdate_t {
  unsigned char  day;       /* 1-31 */
  unsigned char  month;     /* 1-12 */
  unsigned short year;      /* 1980-2099 */
  unsigned char  dayofweek; /* 0-6, 0=Sunday */
};
#define dosdate_t _dosdate_t

struct _dostime_t {
  unsigned char hour;     /* 0-23 */
  unsigned char minute;   /* 0-59 */
  unsigned char second;   /* 0-59 */
  unsigned char hsecond;  /* 0-99 */
};
#define dostime_t _dostime_t

struct _find_t {
  char reserved[21];
  unsigned char attrib;
  unsigned short wr_time;
  unsigned short wr_date;
  ULONG32 size;
  char name[256];
} __attribute__((packed));
#define find_t _find_t
/* make sure that structure packing is correct */
typedef int _DJCHK_FIND_T[(sizeof(struct _find_t)==286)*3 - 1];

struct _diskfree_t {
  unsigned short total_clusters;
  unsigned short avail_clusters;
  unsigned short sectors_per_cluster;
  unsigned short bytes_per_sector;
};
#define diskfree_t _diskfree_t

struct _DOSERROR {
  int  exterror;
  char _class;
  char action;
  char locus;
};
#define DOSERROR _DOSERROR

struct _DOSERROR_STR {
  char *exterror_str;
  char *class_str;
  char *action_str;
  char *locus_str;
};
#define DOSERROR_STR _DOSERROR_STR

unsigned int   _dos_creat(const char *_filename, unsigned int _attr, int *_handle);
unsigned int   _dos_creatnew(const char *_filename, unsigned int _attr, int *_handle);
unsigned int   _dos_open(const char *_filename, unsigned int _mode, int *_handle);
unsigned int   _dos_write(int _handle, const void *_buffer, unsigned int _count, unsigned int *_result);
unsigned int   _dos_read(int _handle, void *_buffer, unsigned int _count, unsigned int *_result);
unsigned int   _dos_close(int _handle);
unsigned int   _dos_commit(int _handle);

unsigned int   _dos_findfirst(const char *_name, unsigned int _attr, struct _find_t *_result);
unsigned int   _dos_findnext(struct _find_t *_result);

void           _dos_getdate(struct _dosdate_t *_date);
unsigned int   _dos_setdate(struct _dosdate_t *_date);
void           _dos_gettime(struct _dostime_t *_time);
unsigned int   _dos_settime(struct _dostime_t *_time);

unsigned int   _dos_getftime(int _handle, unsigned int *_p_date, unsigned int *_p_time);
unsigned int   _dos_setftime(int _handle, unsigned int _date, unsigned int _time);
unsigned int   _dos_getfileattr(const char *_filename, unsigned int *_p_attr);
unsigned int   _dos_setfileattr(const char *_filename, unsigned int _attr);

void           _dos_getdrive(unsigned int *_p_drive);
void           _dos_setdrive(unsigned int _drive, unsigned int *_p_drives);
unsigned int   _dos_getdiskfree(unsigned int _drive, struct _diskfree_t *_diskspace);

int            _dosexterr(struct _DOSERROR *_p_error);
#define dosexterr(_ep) _dosexterr(_ep)
int            _dostrerr(struct _DOSERROR *_p_error, struct _DOSERROR_STR *_p_str);
#define dostrerr(_ep,_sp) _dostrerr(_ep,_sp)

#define int386(_i, _ir, _or)         int86(_i, _ir, _or)
#define int386x(_i, _ir, _or, _sr)   int86x(_i, _ir, _or, _sr)

#ifdef __cplusplus
}
#endif

#endif /* !_POSIX_SOURCE */
#endif /* !__STRICT_ANSI__ */
#endif /* !__dj_ENFORCE_ANSI_FREESTANDING */

#ifndef __dj_ENFORCE_FUNCTION_CALLS
#endif /* !__dj_ENFORCE_FUNCTION_CALLS */

#endif /* !__dj_include_dos_h_ */
