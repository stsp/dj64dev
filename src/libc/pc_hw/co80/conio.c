/* Copyright (C) 2018 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2017 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2002 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1999 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1997 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <libc/stubs.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <pc.h>
#include <errno.h>
#include <unistd.h>
#include <go32.h>
#include <dpmi.h>
#include <libc/farptrgs.h>
#include <conio.h>
#include <libc/bss.h>
#include <libc/unconst.h>

int _wscroll = 1;

int directvideo = 1;  /* We ignore this */

static void refreshvirtualscreen(int c, int r, int count);
static void mayrefreshline(int c, int r, int *srow, int *scol, int *ecol);
static void setcursor(unsigned int shape);
static int getvideomode(void);
static void bell(void);
static int get_screenattrib(void);
static int isEGA(void);
static int _scan_getche(FILE *fp);
static int _scan_ungetch(int c, FILE *fp);

#define DBGGTINFO   0

static unsigned ScreenAddress = 0xb8000UL; /* initialize just in case */
static unsigned short ScreenVirtualSegment = 0; /* 0: non DOS/V */
static unsigned short ScreenVirtualOffset = 0;  /* !0: DOS/V virtual VRAM address */
static struct text_info txinfo;
static int ungot_char;
static int char_avail = 0;

static int adapter_type = -1;       /* 1: EGA, 2: VGA/PGA/MCGA, else 0 */
static int font_seg = -1;           /* segment of DOS buffer for 8x10 font */
static unsigned last_mode = 0xffff; /* video mode when before program start */
static int oldattrib =  -1;         /* text attribute before program start */

static int intense_bg_mode;         /* non-zero if high bit is for bright bg */

static int conio_count = -1;

#define VIDADDR(r,c) (ScreenAddress + 2*(((r) * txinfo.screenwidth) + (c)))
#define VOFFSET(r,c) (ScreenVirtualOffset + 2*(((r) * txinfo.screenwidth) + (c)))

static void
refreshvirtualscreen(int c, int r, int count)
{
  __dpmi_regs regs = {0};

  regs.x.es = ScreenVirtualSegment;
  regs.x.di = VOFFSET(r, c);
  regs.h.ah = 0xff;	/* Refresh Screen */
  regs.x.cx = count;	/* number of characters */
  __dpmi_int(0x10, &regs);
}

static void
mayrefreshline(int c, int r, int *srow, int *scol, int *ecol)
{
  if (*ecol != *scol)
    refreshvirtualscreen(*scol, *srow, *ecol-*scol);
  *srow = r;
  *scol = *ecol = c;
}

int
puttext(int c, int r, int c2, int r2, void *buf)
{
  short *cbuf = (short *)buf;
  /* we should check for valid parameters, and maybe return 0 */
  r--, r2--, c--, c2--;
  for (; r <= r2; r++)
  {
    dosmemput(cbuf, (c2-c+1)*2, VIDADDR(r, c));
    cbuf += c2-c+1;
    if (ScreenVirtualSegment != 0)
      refreshvirtualscreen(c, r, c2-c+1);
  }
  return 1;
}

int
_conio_gettext(int c, int r, int c2, int r2, void *buf)
{
  short *cbuf = (short *)buf;
  /* we should check for valid parameters, and maybe return 0 */
  r--, r2--, c--, c2--;
  for (; r <= r2; r++)
  {
    dosmemget(VIDADDR(r, c), (c2-c+1)*2, cbuf);
    cbuf += c2-c+1;
  }
  return 1;
}

void
gotoxy(int col, int row)
{
  ScreenSetCursor(row + txinfo.wintop - 2, col + txinfo.winleft - 2);
  txinfo.curx = col;
  txinfo.cury = row;
}

int
wherex(void)
{
  int row, col;

  ScreenGetCursor(&row, &col);

  return col - txinfo.winleft + 2;
}

int
wherey(void)
{
  int row, col;

  ScreenGetCursor(&row, &col);

  return row - txinfo.wintop + 2;
}

void
textmode(int mode)
{
  __dpmi_regs regs = {0};
  int mode_to_set = mode;
  if (mode == LASTMODE)
    mode = mode_to_set = last_mode;

  /* Should we support 2 LAST_MODEs in a row?  Right now we do; if not,
     put an ``else'' clause before next line.  */
  last_mode = txinfo.currmode;
  if (mode == C4350)
    /*
     * just set mode 3 and load 8x8 font, idea taken
     * (and code translated from Assembler to C)
     * from Csaba Biegels stdvga.asm
     */
    mode_to_set = 0x03;
  regs.h.ah = 0x00;		/* set mode */
  regs.h.al = mode_to_set;
  __dpmi_int(0x10, &regs);
  if (mode == C80 || mode == BW80 || mode == C4350)
  {
    if (isEGA())
    {
      /*
       * enable cursor size emulation, see Ralf Browns
       * interrupt list
       */
      regs.h.ah = 0x12;
      regs.h.bl = 0x34;
      regs.h.al = 0x00;		/* 0: enable (1: disable) */
      __dpmi_int(0x10, &regs);
    }
  }
  if (mode == C4350)
  {
    if (!isEGA())
      return;
    /* load 8x8 font */
    regs.x.ax = 0x1112;
    regs.x.bx = 0;
    __dpmi_int(0x10, &regs);
  }
  /*    _setcursortype(_NORMALCURSOR); */
  /* reinitialize txinfo structure to take into account new mode */
  gppconio_init();
#if 0
  /*
   * For mode C4350 the screen is not cleared on my OAK-VGA.
   * Should we clear it here? TURBOC doesn't so we don't bother either.
   */
  clrscr();
#endif
}

void
textattr(int attr)
{
  txinfo.attribute = ScreenAttrib = (unsigned char)attr;
}

void
textcolor(int color)
{
  /* strip blinking (highest) bit and textcolor */
  int bg = ScreenAttrib & (intense_bg_mode ? 0xf0 : 0x70);
  txinfo.attribute = (bg | (color & (intense_bg_mode ? 0x0f : 0x8f)));
  ScreenAttrib = txinfo.attribute;
}

void
textbackground(int color)
{
  /* strip background color from ScreenAttrib, keep blinking bit */
  int fg = ScreenAttrib & (intense_bg_mode ? 0x0f : 0x8f);

  /* high intensity background colors (>7) are not allowed, unless
     intense_bg_mode is on, so we strip 0x08 bit (and higher bits) of color */
  color &= (intense_bg_mode ? 0x0f : 0x07);
  txinfo.attribute = ScreenAttrib = (fg | (color << 4));
}

void
highvideo(void)
{
  txinfo.attribute = (ScreenAttrib |= 0x08);
}

void
lowvideo(void)
{
  txinfo.attribute = (ScreenAttrib &= 0xf7);
}

void
normvideo(void)
{
  txinfo.attribute = ScreenAttrib = txinfo.normattr;
}

void
_setcursortype(int type)
{
  unsigned cursor_shape;
  switch (type)
  {
  case _NOCURSOR:
    cursor_shape = 0x2000;
    break;
  case _SOLIDCURSOR:
    cursor_shape = 0x0007;
    break;
    /*      case _NORMALCURSOR: */
  default:
    cursor_shape = 0x0607;
    break;
  }
  setcursor(cursor_shape);
}

static void
setcursor(unsigned int cursor_shape)
/* Sets the shape of the cursor */
{
  __dpmi_regs regs = {0};

  regs.h.ah = 1;
  regs.x.cx = cursor_shape;
  __dpmi_int(0x10, &regs);
} /* setcursor */

static void
getwincursor(int *row, int *col)
{
  ScreenGetCursor(row, col);
}

void
clreol(void)
{
  short   image[ 256 ];
  short   val = ' ' | (ScreenAttrib << 8);
  int     c, row, col, ncols;

  getwincursor(&row, &col);
  ncols = txinfo.winright - col;

  for (c = 0; c < ncols; c++)
    image[ c ] = val;

  puttext(col + 1, row + 1, txinfo.winright, row + 1, image);
}

static void
fillrow(int row, int left, int right, int fill)
{
  int col;
  short filler[right-left+1];

  for (col = left; col <= right; col++)
    filler[col-left] = fill;
  dosmemput(filler, (right-left+1)*2, VIDADDR(row, left));
  if (ScreenVirtualSegment != 0)
    refreshvirtualscreen(left, row, right-left+1);
}

void
clrscr(void)
{
  short filler[txinfo.winright - txinfo.winleft + 1];
  int row, col;
  for (col=0; col < txinfo.winright - txinfo.winleft + 1; col++)
    filler[col] = ' ' | (ScreenAttrib << 8);
  for (row=txinfo.wintop-1; row < txinfo.winbottom; row++)
    {
      dosmemput(filler, (txinfo.winright - txinfo.winleft + 1)*2,
		VIDADDR(row, txinfo.winleft - 1));
      if (ScreenVirtualSegment != 0)
	refreshvirtualscreen(txinfo.winleft - 1, row, txinfo.winright - txinfo.winleft + 1);
    }
  gotoxy(1, 1);
}

int
putch(int c)
{
  int     row, col;

  ScreenGetCursor(&row, &col);

  /*  first, handle the character */
  if (c == '\n')
  {
    row++;
  }
  else if (c == '\r')
  {
    col = txinfo.winleft - 1;
  }
  else if (c == '\b')
  {
    if (col > txinfo.winleft - 1)
      col--;
    else if (row > txinfo.wintop -1)
    {
      /*
       * Turbo-C ignores this case; we are smarter.
       */
      row--;
      col = txinfo.winright-1;
    }
  }
  else if (c == 0x07)
    bell();
  else {
    ScreenPutChar(c, ScreenAttrib, col, row);
    if (ScreenVirtualSegment != 0)
      refreshvirtualscreen(col, row, 1);
    col++;
  }

  /* now, readjust the window     */

  if (col >= txinfo.winright)
  {
    col = txinfo.winleft - 1;
    row++;
  }

  if (row >= txinfo.winbottom)
  {
    /* scrollwin(0, txinfo.winbottom - txinfo.wintop, 1); */
    if (_wscroll)
    {
      ScreenSetCursor(txinfo.wintop-1,0);
      delline();
    }
    row--;
  }

  ScreenSetCursor(row, col);
  txinfo.cury = row - txinfo.wintop + 2;
  txinfo.curx = col - txinfo.winleft + 2;
  return c;
}

int
getche(void)
{
  if (char_avail)
    /*
     * We don't know, wether the ungot char was already echoed
     * we assume yes (for example in cscanf, probably the only
     * place where ungetch is ever called.
     * There is no way to check for this really, because
     * ungetch could have been called with a character that
     * hasn't been got by a conio function.
     * We don't echo again.
     */
    return(getch());
  return (putch(getch()));
}

int
getch(void)
{
  __dpmi_regs regs = {0};
  int c;

  /* Flush any buffered output, otherwise they might get confusing
     out-of-order execution (and we get to answer FAQs).  */
  if (isatty(0))
  {
    if ((stdout->_flag&_IOLBF) && isatty(fileno(stdout)))
      fflush(stdout);
    if ((stderr->_flag&_IOLBF) && isatty(fileno(stderr)))
      fflush(stderr);
  }
  if (char_avail)
  {
    c = ungot_char;
    char_avail = 0;
  }
  else
  {
    regs.x.ax = 0x0700;
    __dpmi_int(0x21, &regs);
    c = regs.h.al;
  }
  return(c);
}

int
ungetch(int c)
{
  if (char_avail)
    return(EOF);
  ungot_char = c;
  char_avail = 1;
  return(c);
}

/*
 * kbhit from libc in libsrc/c/dos/kbhit.s doesn't check
 * for ungotten chars, so we have to provide a new one
 * Don't call it kbhit, rather use a new name (_conio_kbhit)
 * and do a #define kbhit _conio_kbhit in gppconio.h.
 * The old kbhit still can be used if gppconio.h
 * is not included of after #undef kbhit
 * If you don't use ungetch (directly or indirectly by cscanf)
 * both kbhit and _conio_kbhit are the same.
 * So this shouldn't cause any trouble with previously written
 * source, because ungetch wasn't available.
 * The only problem might be, if anybody just included gppconio.h
 * and has not linked with libpc, (I can't think of a good reason
 * for this). This will result a link error (undefined symbol _conio_kbhit).
 */

#undef kbhit  /* want to be able to call kbhit from libc */

int
_conio_kbhit(void)
{
  if (char_avail)
    return(1);
  else
    return(kbhit());
}

/*
 * The next two functions are needed by cscanf
 */
static int
_scan_getche(FILE *fp)
{
  return(getche());
}

static int
_scan_ungetch(int c, FILE *fp)
{
  return(ungetch(c));
}


void
insline(void)
{
  int row, col, left, right, nbytes, bot, fill;
  ScreenGetCursor(&row, &col);
  left = txinfo.winleft - 1;
  right = txinfo.winright - 1;
  nbytes = (right-left+1)*2;
  bot = txinfo.winbottom-1;
  fill = ' ' | (ScreenAttrib << 8);
  while (bot > row)
  {
    fmemcpy12(
	     DP(_dos_ds, VIDADDR(bot, left)),
	     DP(_dos_ds, VIDADDR(bot-1, left)),
	     nbytes);
    if (ScreenVirtualSegment != 0)
      refreshvirtualscreen(left, bot-1, nbytes/2);
    bot--;
  }
  fillrow(row,left,right,fill);
}


void
delline(void)
{
  int row, col, left, right, nbytes, bot, fill;
  ScreenGetCursor(&row, &col);
  left = txinfo.winleft - 1;
  right = txinfo.winright - 1;
  nbytes = (right-left+1)*2;
  bot = txinfo.winbottom-1;
  fill = ' ' | (ScreenAttrib << 8);
  while(row < bot)
  {
    fmemcpy12(
	     DP(_dos_ds, VIDADDR(row, left)),
	     DP(_dos_ds, VIDADDR(row+1, left)),
	     nbytes);
    if (ScreenVirtualSegment != 0)
      refreshvirtualscreen(left, row, nbytes/2);
    row++;
  }
  fillrow(bot,left,right,fill);
}


void
window(int left, int top, int right, int bottom)
{
  if (top < 1 || left < 1 || right > txinfo.screenwidth ||
      bottom > txinfo.screenheight)
    return;

  txinfo.wintop = top;
  txinfo.winleft = left;
  txinfo.winright = right;
  txinfo.winbottom = bottom;
  gotoxy(1,1);
}


int
cputs(const char *s)
{
  int     row, col,c;
  const unsigned char *ss = (const unsigned char *)s;
  short *viaddr;
  short sa = ScreenAttrib << 8;
  int srow, scol, ecol;
  ScreenGetCursor(&row, &col);
  viaddr = (short *)(uintptr_t)VIDADDR(row,col);
  /*
   * DOS/V: simply it refreshes screen between scol and ecol when cursor moving.
   */
  srow = row;
  scol = ecol = col;

  /*
   * Instead of just calling putch; we do everything by hand here,
   * This is much faster. We don't move the cursor after each character,
   * only after the whole string is written, because ScreenSetCursor
   * needs to long because of switching to real mode needed with djgpp.
   * You won't recognize the difference.
   */
  while ((c = *ss++))
  {
    /*  first, handle the character */
    if (c == '\n')
    {
      row++;
      viaddr += txinfo.screenwidth;
      if (ScreenVirtualSegment != 0)
	mayrefreshline(col, row, &srow, &scol, &ecol);
    }
    else if (c == '\r')
    {
      col = txinfo.winleft - 1;
      viaddr = (short *)(uintptr_t)VIDADDR(row,col);
      if (ScreenVirtualSegment != 0)
	mayrefreshline(col, row, &srow, &scol, &ecol);
    }
    else if (c == '\b')
    {
      if (col > txinfo.winleft-1)
      {
	col--;
	viaddr--;
      }
      else if (row > txinfo.wintop -1)
      {
	/*
	 * Turbo-C ignores this case. We want to be able to
	 * edit strings with backspace in gets after
	 * a linefeed, so we are smarter
	 */
	row--;
	col = txinfo.winright-1;
	viaddr = (short *)(uintptr_t)VIDADDR(row,col);
      }
      if (ScreenVirtualSegment != 0)
	mayrefreshline(col, row, &srow, &scol, &ecol);
    }
    else if (c == 0x07)
      bell();
    else {
      short q = c | sa;
      dosmemput(&q, 2, (uintptr_t)viaddr);
      viaddr++;
      col++;
      ecol++;
    }

    /* now, readjust the window     */

    if (col >= txinfo.winright) {
      col = txinfo.winleft - 1;
      row++;
      viaddr = (short *)(uintptr_t)VIDADDR(row,col);
      if (ScreenVirtualSegment != 0)
	mayrefreshline(col, row, &srow, &scol, &ecol);
    }

    if (row >= txinfo.winbottom) {
      /* refresh before scroll */
      if (ScreenVirtualSegment != 0)
	mayrefreshline(col, row, &srow, &scol, &ecol);
      if (_wscroll)
      {
	ScreenSetCursor(txinfo.wintop-1,0); /* goto first line in window */
	delline();		/* and delete it */
      }
      row--;
      srow--;
      viaddr -= txinfo.screenwidth;
    }
  }

  /* refresh the rest of cols */
  if (ScreenVirtualSegment != 0)
    mayrefreshline(col, row, &srow, &scol, &ecol);
  ScreenSetCursor(row, col);
  txinfo.cury = row - txinfo.wintop + 2;
  txinfo.curx = col - txinfo.winleft + 2;
  return(*(--ss));
}


int
cprintf(const char *fmt, ...)
{
  int     cnt;
  char    buf[ 2048 ];		/* this is buggy, because buffer might be too small. */
  va_list ap;

  va_start(ap, fmt);
  cnt = vsprintf(buf, fmt, ap);
  va_end(ap);

  cputs(buf);
  return cnt;
}

char *
cgets(char *string)
{
  unsigned len = 0;
  unsigned int maxlen_wanted;
  char *sp;
  int c;
  /*
   * Be smart and check for NULL pointer.
   * Don't know wether TURBOC does this.
   */
  if (!string)
    return(NULL);
  maxlen_wanted = (unsigned int)((unsigned char)string[0]);
  sp = &(string[2]);
  /*
   * Should the string be shorter maxlen_wanted including or excluding
   * the trailing '\0' ? We don't take any risk.
   */
  while(len < maxlen_wanted-1)
  {
    c=getch();
    /*
     * shold we check for backspace here?
     * TURBOC does (just checked) but doesn't in cscanf (thats harder
     * or even impossible). We do the same.
     */
    if (c == '\b')
    {
      if (len > 0)
      {
	cputs("\b \b");		/* go back, clear char on screen with space
				   and go back again */
	len--;
	sp[len] = '\0';		/* clear the character in the string */
      }
    }
    else if (c == '\r')
    {
      sp[len] = '\0';
      break;
    }
    else if (c == 0)
    {
      /* special character ends input */
      sp[len] = '\0';
      ungetch(c);		/* keep the char for later processing */
      break;
    }
    else
    {
      sp[len] = putch(c);
      len++;
    }
  }
  sp[maxlen_wanted-1] = '\0';
  string[1] = (char)((unsigned char)len);
  return(sp);
}

int
cscanf(const char *fmt, ...)
{
  va_list args;
  int ret;

  va_start(args, fmt);
  ret = _doscan_low(NULL, _scan_getche, _scan_ungetch, fmt, args);
  va_end(args);

  return(ret);
}

int
movetext(int left, int top, int right, int bottom, int dleft, int dtop)
{
  char *buf = (char *)alloca((right - left + 1) * (bottom - top + 1) * 2);

  _conio_gettext(left, top, right, bottom, buf);
  puttext(dleft, dtop, dleft + right - left, dtop + bottom - top, buf);
  return 1;
}

static void
_gettextinfo(struct text_info *t)
{
  int row, col;

  t->winleft = t->wintop = 1;
  t->winright = t->screenwidth = ScreenCols();
  t->winbottom = t->screenheight = ScreenRows();
  ScreenAttrib = t->attribute = get_screenattrib();
  t->normattr = oldattrib;
  t->currmode = getvideomode();
  ScreenGetCursor(&row, &col);
  t->curx = col+1;
  t->cury = row+1;
#if DBGGTINFO
  printf("left=%2d,right=%2d,top=%2d,bottom=%2d\n",t->winleft,
	 t->winright,t->wintop,t->winbottom);
  printf("scrht=%2d,scrwid=%2d,norm=%2x,mode=%2d,x=%2d,y=%2d\n",
	 t->screenheight, t->screenwidth, t->normattr, t->currmode,
	 t->curx, t->cury);
#endif
}

void
gettextinfo(struct text_info *t)
{
  *t = txinfo;
#if DBGGTINFO
  printf("left=%2d,right=%2d,top=%2d,bottom=%2d\n",t->winleft,
	 t->winright,t->wintop,t->winbottom);
  printf("scrht=%2d,scrwid=%2d,norm=%2x,mode=%2d,x=%2d,y=%2d\n",
	 t->screenheight, t->screenwidth, t->normattr, t->currmode,
	 t->curx, t->cury);
#endif
}

static int
getvideomode(void)
{
  int mode = ScreenMode();
  /*
   * in mode C80 we might have loaded a different font
   */
  if (mode == C80)
    if (ScreenRows() > 25)
      mode = C4350;
  return(mode);
}


static void
bell(void)
{
  __dpmi_regs regs = {0};
  regs.h.ah = 0x0e;		/* write */
  regs.h.al = 0x07;		/* bell */
  __dpmi_int(0x10, &regs);
}

static int
get_screenattrib(void)
{
  __dpmi_regs regs = {0};
  regs.h.ah = 0x08;		/* read character and attribute */
  regs.h.bh = 0;		/* video page 0 */
  __dpmi_int(0x10, &regs);
  return(regs.h.ah & 0x7f);	/* strip highest (BLINK) bit */
}

/* Check if we have at least EGA.
   Return 1 if EGA, 2 if VGA/PGA/MCGA, else 0. */
static int
isEGA(void)
{
  if (adapter_type == -1)
    {
      __dpmi_regs regs = {0};

      /* Get display combination code.  */
      regs.x.ax = 0x1a00;
      __dpmi_int(0x10, &regs);
      if (regs.h.al == 0x1a)    /* if Int 10h/AX=1A00h supported */
        switch (regs.h.bl)
          {
            case 4:
            case 5:
                adapter_type = 1; /* EGA */
                break;
            case 6:             /* PGA */
            case 7:             /* VGA */
            case 8:             /* VGA */
            case 10:            /* MCGA */
            case 11:            /* MCGA */
            case 12:            /* MCGA */
                adapter_type = 2;
                break;
            default:
                adapter_type = 0;
          }

      else
        {
          /* Int 10h/AX=1A00h not supported.  Try getting EGA info.  */
          regs.h.ah = 0x12;
          regs.h.bl = 0x10;
          regs.h.bh = 0xff;
          __dpmi_int(0x10, &regs);
          adapter_type = (regs.h.bh != 0xff);
        }
    }

  return adapter_type;
}

/* Set screen scan lines and load appropriate font.
   SCAN_LINES and FONT are as required by Int 10h functions 12h and 11h */
static void
set_scan_lines_and_font(int scan_lines, int font)
{
  __dpmi_regs regs = {0};

  /* Set 200/350/400 scan lines.  */
  regs.h.ah = 0x12;
  regs.h.al = scan_lines;       /* 0: 200, 1: 350, 2: 400 */
  regs.h.bl = 0x30;
  __dpmi_int(0x10, &regs);

  /* Scan lines setting only takes effect when video mode is set.  */
  regs.x.ax = txinfo.currmode == 7 ? 7 : 3;
  __dpmi_int(0x10, &regs);

  /* Load a ROM BIOS font (0x11: 8x14, 0x12: 8x8, 0x14: 8x16).  */
  regs.h.bl = 0;                /* block zero */
  regs.h.ah = 0x11;
  regs.h.al = font & 0xff;
  __dpmi_int(0x10, &regs);
}

/* Stretch a 8x8 font to the 8x10 character box.  This is required to
   use 80x40 mode on a VGA or 80x35 mode on an EGA, because the character
   box is 10 lines high, and the ROM BIOS doesn't have an appropriate font.
   So we create one from the 8x8 font by adding an extra blank line
   from each side.  */
static void
maybe_create_8x10_font(void)
{
  unsigned long src, dest, i, j;

  if (font_seg == -1)
    {
      __dpmi_regs regs = {0};
      int buf_pm_sel;

      /* Allocate buffer in conventional memory. */
      font_seg = __dpmi_allocate_dos_memory(160, &buf_pm_sel);

      if (font_seg == -1)
        return;

      /* Get the pointer to the 8x8 font table.  */
      regs.h.bh = 3;
      regs.x.ax = 0x1130;
      __dpmi_int(0x10, &regs);
      src =  ( ( (unsigned)regs.x.es ) << 4 ) + regs.x.bp;
      dest = ( (unsigned)font_seg ) << 4;

      /* Now copy the font to our table, stretching it to 8x10. */
      _farsetsel(_dos_ds);
      for (i = 0; i < 256; i++)
        {
          /* Fill first extra scan line with zeroes. */
          _farnspokeb(dest++, 0);

          for (j = 0; j < 8; j++)
            {
              unsigned char val = _farnspeekb(src++);

              _farnspokeb(dest++, val);
            }

          /* Fill last extra scan line with zeroes. */
          _farnspokeb(dest++, 0);
        }
    }
}

/* Load the 8x10 font we created into character generator RAM.  */
static void
load_8x10_font(void)
{
  __dpmi_regs regs = {0};

  maybe_create_8x10_font();         /* create if needed */
  if (font_seg == -1)
    return;
  regs.x.es = font_seg;             /* pass pointer to our font in ES:BP */
  regs.x.bp = 0;
  regs.x.dx = 0;                    /* 1st char: ASCII 0 */
  regs.x.cx = 256;                  /* 256 chars */
  regs.h.bh = 10;                   /* 10 points per char */
  regs.h.bl = 0;                    /* block 0 */
  regs.x.ax = 0x1110;
  __dpmi_int(0x10, &regs);
}

/* Set screen scan lines and load 8x10 font.
   SCAN_LINES is as required by Int 10h function 12h. */
static void
set_scan_lines_and_8x10_font(int scan_lines)
{
  __dpmi_regs regs = {0};

  regs.h.bl = 0x30;
  regs.h.ah = 0x12;
  regs.h.al = scan_lines;           /* 0: 200, 1: 350, 2: 400 */
  __dpmi_int(0x10, &regs);

  /* Set video mode, so that scan lines we set will take effect.  */
  regs.x.ax = txinfo.currmode == 7 ? 7 : 3;
  __dpmi_int(0x10, &regs);

  /* Load our 8x10 font and enable intensity bit.  */
  load_8x10_font();
}

/* Switch to screen lines given by NLINES.  */
void
_set_screen_lines(int nlines)
{
  __dpmi_regs regs = {0};
  switch (nlines)
    {
      case 25:
          if (adapter_type)
            {
              /* Set 350 scan lines for EGA, 400 for VGA.  */
              regs.h.bl = 0x30;
              regs.h.ah = 0x12;
              regs.h.al = (adapter_type > 1 ? 2 : 1);
              __dpmi_int(0x10, &regs);

              /* Load ROM BIOS font: 8x14 for EGA, 8x16 for VGA.  */
              regs.h.bl = 0;
              regs.h.ah = 0x11;
              regs.h.al = (adapter_type > 1 ? 0x14 : 0x11);
              __dpmi_int(0x10, &regs);
            }

          /* Set video mode.  */
          regs.x.ax = txinfo.currmode == 7 ? 7 : 3;
          __dpmi_int(0x10, &regs);
          break;
      case 28:      /* VGA only */
          if (adapter_type > 1)
            set_scan_lines_and_font(2, 0x11);
          break;
      case 35:      /* EGA or VGA */
          if (adapter_type)
            set_scan_lines_and_8x10_font(1);
          break;
      case 40:      /* VGA only */
          if (adapter_type > 1)
            set_scan_lines_and_8x10_font(2);
          break;
      case 43:      /* EGA or VGA */
          if (adapter_type)
            set_scan_lines_and_font(1, 0x12);
          break;
      case 50:      /* VGA only */
          if (adapter_type > 1)
            set_scan_lines_and_font(2, 0x12);
          break;
    }

  _gettextinfo(&txinfo);
}

void
blinkvideo(void)
{

  /* Set intensity/blinking bit to BLINKING.  */
  __dpmi_regs regs = {0};
  regs.h.bl = 1;
  regs.x.ax = 0x1003;
  __dpmi_int(0x10, &regs);
  intense_bg_mode = (_farpeekb(_dos_ds, 0x465) & 0x20) == 0;
}

void
intensevideo(void)
{

  /* Set intensity/blinking bit to INTENSE (bright background).  */
  __dpmi_regs regs = {0};
  regs.h.bl = 0;
  regs.x.ax = 0x1003;
  __dpmi_int(0x10, &regs);
  intense_bg_mode = (_farpeekb(_dos_ds, 0x465) & 0x20) == 0;
}

void
gppconio_init(void)
{
  __dpmi_regs regs = {0};

  /* Force initialization in restarted programs (emacs).  */
  if (conio_count != __bss_count)
    {
      conio_count = __bss_count;
      oldattrib = -1;
      last_mode = 0xffff;
      font_seg = -1;
    }

  (void)isEGA();    /* sets the global ADAPTER_TYPE */

  if (oldattrib == -1)
    oldattrib = get_screenattrib();
  if (last_mode == 0xffff)
    last_mode = getvideomode();
  _gettextinfo(&txinfo);
  if (txinfo.currmode == 7)	/* MONO */
    ScreenAddress = 0xb0000UL;
  else
    ScreenAddress = 0xb8000UL;
  intense_bg_mode = (_farpeekb(_dos_ds, 0x465) & 0x20) == 0;

  regs.x.es = regs.x.di = 0;	/* Dummy for checking */
  regs.h.ah = 0xfe;		/* Get Video Buffer */
  __dpmi_int(0x10, &regs);
  ScreenVirtualSegment = regs.x.es;
  ScreenVirtualOffset = regs.x.di;
  if (ScreenVirtualSegment != 0)
    ScreenAddress = (ScreenVirtualSegment << 4UL) + ScreenVirtualOffset;
  ScreenPrimary = ScreenAddress;

#if 0
  /* Why should gppconio_init() restore OLDATTRIB?  I think it
     shouldn't, because this causes change of colors when all
     user wants is to update TXINFO.  And besides, _gettextinfo()
     above has already set ScreenAttrib.  */
  ScreenAttrib = txinfo.normattr = txinfo.attribute = oldattrib;
#endif
}

__asm__(".section .ctor; .long _gppconio_init; .section .text");
