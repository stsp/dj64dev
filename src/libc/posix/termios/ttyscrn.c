/* Copyright (C) 2015 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2002 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details */
/* This file determines how characters are placed on the screen
   when the termios emulation is active.  */
#include <libc/stubs.h>
#include <termios.h>
#include <go32.h>
#include <dpmi.h>
#include <libc/ttyprvt.h>
#include <libc/farptrgs.h>

struct direct_private_info
{
  unsigned long video_buffer;
  unsigned long video_buffer_segment;
  unsigned long video_buffer_offset;
};

/* Functions to output directly to the video buffer.  */
static inline unsigned long get_video_offset(int col, int row);
static void direct_init(void);
static void direct_write_ch(unsigned char ch, int *col, int *row);
static void direct_put_ch(unsigned char ch);
static void direct_put_ch_at(unsigned char ch);
static void direct_puts(const unsigned char *s);
static void direct_clear(int x1, int y1, int x2, int y2);
static void direct_scroll_up(int y1, int y2, int delta);
static void direct_scroll_down(int y1, int y2, int delta);
static void direct_scroll_left(int y, int x1, int x2, int xdst);
static void direct_scroll_right(int y, int x1, int x2, int xdst);
static void direct_scroll_forward(int x1, int y1, int x2, int y2, int xdst, int ydst);
static void direct_scroll_backward(int x1, int y1, int x2, int y2, int xdst, int ydst);
static void direct_refresh_virtual_screen(int col, int row, int count);

/* Functions to use the video BIOS for screen output.  */
static void vbios_write_ch(unsigned char ch, int *col, int *row);
static void vbios_put_ch(unsigned char ch);
static void vbios_put_ch_at(unsigned char ch);
static void vbios_puts(const unsigned char *s);
static void vbios_clear(int x1, int y1, int x2, int y2);
static void vbios_scroll_up(int y1, int y2, int delta);
static void vbios_scroll_down(int y1, int y2, int ydst);
static void vbios_scroll_left(int y, int x1, int x2, int xdst);
static void vbios_scroll_right(int y, int x1, int x2, int xdst);

static struct direct_private_info direct_info;

/* This driver outputs directly to the video buffer.  */
struct tty_screen_interface __tty_direct_intface =
  { direct_init, direct_write_ch, direct_put_ch, direct_put_ch_at,
    direct_puts, direct_scroll_up, direct_scroll_down,
    direct_scroll_left, direct_scroll_right, direct_clear };

/* This driver uses only the video bios routines.  */
struct tty_screen_interface __tty_vbios_intface =
  { NULL, vbios_write_ch, vbios_put_ch, vbios_put_ch_at,
    vbios_puts, vbios_scroll_up, vbios_scroll_down,
    vbios_scroll_left, vbios_scroll_right, vbios_clear };


/* Functions to support direct output to the video buffer.  */

/* Calculate the offset into the video buffer given the column and row.  */
static inline unsigned long
get_video_offset(int col, int row)
{
  return direct_info.video_buffer + 2 * (row * (__tty_screen.max_col + 1)
                                         + col);
}

/* Initialize the direct video interface.  */
static void
direct_init(void)
{
  __dpmi_regs r = {0};

  /* Determine the video offset.  */
  direct_info.video_buffer = _go32_info_block.linear_address_of_primary_screen;

  /* DOS/V support.  */
  r.x.es = 0;
  r.x.di = 0;
  r.h.ah = 0xfe;
  __dpmi_int(0x10, &r);
  direct_info.video_buffer_segment = r.x.es;
  direct_info.video_buffer_offset = r.x.di;
  if (direct_info.video_buffer_segment != 0)
  {
    direct_info.video_buffer = direct_info.video_buffer_segment << 4UL;
    direct_info.video_buffer += direct_info.video_buffer_offset;
  }
}

static inline void
__bios_output(unsigned char ch)
{
  __dpmi_regs r = {0};

  if (ch == 0xff)
    return;

  r.h.ah = 0x0e;
  r.h.al = ch;
  r.h.bh = __tty_screen.active_page;
  __dpmi_int (16, &r);
}

/* Write out a character.  Update col and row, but don't move the cursor
   to avoid the protected mode->real mode->protected transition.  */
static void
direct_write_ch(unsigned char ch, int *col, int *row)
{
  if (ch == '\n')
    ++(*row);
  else if (ch == '\r')
    *col = 0;
  else if (ch == '\b')  /* Emulate the way bios handles backspace.  */
  {
    if (*col > 0)
      --(*col);
    else if (*row > 0)
    {
      --(*row);
      *col = __tty_screen.max_col;
    }
  }
  else if (ch == 0x07)  /* Ring the bell.  */
    __bios_output((unsigned char)ch);
  else
  {
    unsigned long ptr = get_video_offset(*col, *row);
    _farpokew(_dos_ds, ptr, ch | (__tty_screen.attrib << 8));

    /* DOS/V support.  */
    if (direct_info.video_buffer_segment)
      direct_refresh_virtual_screen(*col, *row, 1);

    ++(*col);
  }

  /* Handle going past the last column.  */
  if (*col > __tty_screen.max_col)
  {
    *col = 0;
    ++(*row);
  }

  /* Scroll when the current row passes the bottom row.  */
  if (*row > __tty_screen.max_row)
  {
    direct_scroll_backward(0, 1, __tty_screen.max_col, __tty_screen.max_row,
                           0, 0);
    --(*row);
  }
}

/* Write out the character and move the cursor.  */
static void direct_put_ch(unsigned char ch)
{
  int col, row;

  __tty_screen.get_cursor(&col, &row);
  direct_write_ch(ch, &col, &row);
  __tty_screen.set_cursor(col, row);
}

/* Write out the character, but don't move the cursor.  */
static void direct_put_ch_at(unsigned char ch)
{
  int col, row;
  unsigned long ptr;

  __tty_screen.get_cursor(&col, &row);

  ptr = get_video_offset(col, row);
  _farpokew(_dos_ds, ptr, ch | (__tty_screen.attrib << 8));

  /* Support DOS/V.  */
  if (direct_info.video_buffer_segment)
    direct_refresh_virtual_screen(col, row, 1);
}

/* Write out a zero-terminated string of characters.  */
static void direct_puts(const unsigned char *s)
{
  int col, row;

  __tty_screen.get_cursor(&col, &row);

  while (*s)
  {
    direct_write_ch(*s, &col, &row);
    ++s;
  }

  __tty_screen.set_cursor(col, row);
}

/* Clear from (x1, y1) to (x2, y2).  */
static void
direct_clear(int x1, int y1, int x2, int y2)
{
  unsigned long ptr, ptr_end;
  unsigned short fill;
  unsigned long fill_l;

  ptr = get_video_offset(x1, y1);
  ptr_end = get_video_offset(x2, y2);

  fill = ' ' | (__tty_screen.attrib << 8);
  fill_l = fill | (fill << 16);

  /* Align the upcoming 4-byte poke on a 4-byte boundary.  */
  _farsetsel(_dos_ds);
  if (ptr % 4 != 0)
  {
    _farnspokew(ptr, fill);
    ptr += 2;
  }

  /* Fill 4 bytes or 2 characters at a time.  */
  while (ptr < ptr_end)
  {
    _farnspokel(ptr, fill_l);
    ptr += 4;
  }
  /* Fill the last character by itself if
     the end isn't on a 4-byte boundary.  */
  if (ptr == ptr_end)
    _farnspokew(ptr, fill);

  /* Support DOS/V virtual screen.  */
  if (direct_info.video_buffer_segment)
    direct_refresh_virtual_screen(x1, y1, (y2 - y1) * __tty_screen.max_col
                                          + (x2 - x1) + 1);
}

/* Move the area (x1, y1), (x2, y2) to
   (xdst, ydst), ((xdst + (x2 - x1)), (ydst + (y2 - y1))). */
static void
direct_scroll_forward(int x1, int y1, int x2, int y2, int xdst, int ydst)
{
  unsigned short fill;
  unsigned long fill_l;
  unsigned long dst_ptr, src_ptr, dst_end, src_end;

  int xdst2, ydst2;

  xdst2 = xdst + (x2 - x1);
  ydst2 = ydst + (y2 - y1);

  /* Compute our beginning and ending offsets.  */
  dst_ptr = get_video_offset(xdst2, ydst2);
  src_ptr = get_video_offset(x2, y2);
  dst_end = get_video_offset(xdst, ydst);
  src_end = get_video_offset(x1, y1);

  fill = ' ' | (__tty_screen.attrib << 8);
  fill_l = fill | (fill << 16);

  /* Align the 4-byte copy at a 4-byte boundary if possible.  */
  _farsetsel(_dos_ds);
  if ((dst_ptr % 4) != 0 && (src_ptr % 4) != 0)
  {
    _farnspokew(dst_ptr, _farnspeekw(src_ptr));
    dst_ptr -= 2;
    src_ptr -= 2;
  }

  /* Copy 4-bytes at a time.  */
  while (dst_ptr >= dst_end)
  {
    _farnspokel(dst_ptr, _farnspeekl(src_ptr));
    dst_ptr -= 4;
    src_ptr -= 4;
  }

  /* If the last character isn't on a 4-byte boundary,
     copy it seperately.  */
  if (dst_end - dst_ptr == 2)
    _farnspokew(dst_end, _farnspeekw(src_end));

  /* Fill in the space vacated.  */
  dst_end -= 2;

  /* Special case for a one character fill.  */
  if (dst_end == src_end)
  {
    _farnspokew(dst_end, fill);
    return;
  }

  dst_end -= 2;

  /* Fill 4-bytes or 2 characters at a time.  */
  while (dst_end >= src_end)
  {
    _farnspokel(dst_end, fill_l);
     dst_end -= 4;
  }

  /* Special handling when last character isn't on a 4-byte boundary.  */
  if (src_end - dst_end == 2)
    _farnspokew(src_end, fill);

  /* Support DOS/V virtual screen.  */
  if (direct_info.video_buffer_segment)
    direct_refresh_virtual_screen(x1, y1, x2 - x1 + 1);

}

/* Move the area (x1, y1), (x2, y2) to
   (xdst, ydst), ((xdst + (x2 - x1)), (ydst + (y2 - y1))). */
static void
direct_scroll_backward(int x1, int y1, int x2, int y2, int xdst, int ydst)
{
  unsigned short fill;
  unsigned long fill_l;
  unsigned long dst_ptr, src_ptr, dst_end, src_end;
  int xdst2, ydst2;

  xdst2 = xdst + (x2 - x1);
  ydst2 = ydst + (y2 - y1);

  /* Compute our beginning and ending offsets.  */
  dst_ptr = get_video_offset(xdst, ydst);
  src_ptr = get_video_offset(x1, y1);
  dst_end = get_video_offset(xdst2, ydst2);
  src_end = get_video_offset(x2, y2);

  /* On to business.  */
  fill = ' ' | (__tty_screen.attrib << 8);
  fill_l = fill | (fill << 16);

  /* Align the 4-byte copy at a 4-byte boundary if possible.  */
  _farsetsel(_dos_ds);
  if ((dst_ptr % 4) != 0 && (src_ptr % 4) != 0)
  {
    _farnspokew(dst_ptr, _farnspeekw(src_ptr));
    dst_ptr += 2;
    src_ptr += 2;
  }

  /* Copy 4-bytes at a time.  */
  while (dst_ptr <= dst_end)
  {
    _farnspokel(dst_ptr, _farnspeekl(src_ptr));
    dst_ptr += 4;
    src_ptr += 4;
  }

  /* If the last character isn't on a 4-byte boundary,
     copy it seperately.  */
  if (dst_ptr - dst_end == 2)
    _farnspokew(dst_end, _farnspeekw(src_end));

  dst_end += 2;

  /* Special case for a one character fill.  */
  if (dst_end == src_end)
  {
    _farnspokew(dst_end, fill);
    return;
  }

  /* Align fill on 4-byte boundary. */
  if ((dst_end % 4) != 0)
  {
    _farnspokew(dst_end, fill);
    dst_end += 2;
  }

  /* Fill 4-bytes or 2 characters at a time.  */
  while (dst_end < src_end)
  {
    _farnspokel(dst_end, fill_l);
     dst_end += 4;
  }

  /* Special handling when last character isn't on a 4-byte boundary.  */
  if (dst_end == src_end)
    _farnspokew(src_end, fill);

  /* Support DOS/V virtual screen.  */
  if (direct_info.video_buffer_segment)
    direct_refresh_virtual_screen(xdst, ydst, x2 - xdst + 1);
}

/* Scroll y1 to y2 up to ydst.  */
static void
direct_scroll_up(int y1, int y2, int ydst)
{
  direct_scroll_backward(0, y1, __tty_screen.max_col, y2, 0, ydst);
}

/* Scroll y1 to y2 down to ydst.  */
static void
direct_scroll_down(int y1, int y2, int ydst)
{
  direct_scroll_forward(0, y1, __tty_screen.max_col, y2, 0, ydst);
}

/* Scroll x1 to x2 left to xdst.  */
static void direct_scroll_left(int y, int x1, int x2, int xdst)
{
  direct_scroll_backward(x1, y, x2, y, xdst, y);
}

/* Scroll x1 to x2 right to xdst.  */
static void direct_scroll_right(int y, int x1, int x2, int xdst)
{
  direct_scroll_forward(x1, y, x2, y, xdst, y);
}

/* DOS/V support.  */
static void
direct_refresh_virtual_screen(int col, int row, int count)
{
  __dpmi_regs regs = {0};

  regs.x.es = direct_info.video_buffer_segment;
  regs.x.di = direct_info.video_buffer_offset;
  regs.x.di += 2 * (row * (__tty_screen.max_col + 1) + col);
  regs.h.ah = 0xff;	/* Refresh Screen */
  regs.x.cx = count;	/* number of characters */
  __dpmi_int(0x10, &regs);
}


/* Functions to support output using the video BIOS.  */

/* Write out a character and update col and row.  The cursor is unavoidably
   updated.  So the overhead of the protected mode->real mode->protected
   transition is present.  */
static void vbios_write_ch(unsigned char ch, int *col, int *row)
{
  static unsigned char attrib_changed;

  __dpmi_regs r = {0};

  /* If a non-default attribute is ever used, then prefer the
     int 0x10 ah=0x0e service because then the attribute under
     the cursor could be anything.  */
  if (attrib_changed == 0
      && (__tty_screen.attrib != __tty_screen.init_attrib))
    ++attrib_changed;

  r.h.al = ch;
  r.h.bh = __tty_screen.active_page;
  r.x.cx = 1;
  if (((__tty_screen.attrib == __tty_screen.init_attrib) && !attrib_changed)
      || (ch == '\r' || ch == '\n' || ch == '\b' || ch == 0x07))
  {
    r.h.ah = 0x0e;
    __dpmi_int(0x10, &r);
    /* So where did the cursor go? */
    __tty_screen.get_cursor(col, row);
  }
  else
  {
    r.h.bl = __tty_screen.attrib;
    r.h.ah = 0x09;
    __dpmi_int(0x10, &r);
    if (++(*col) > __tty_screen.max_col)
    {
      *col = 0;
      ++(*row);
    }
    __tty_screen.set_cursor(*col, *row);
  }
}

/* Write out the character and move the cursor.  */
static void vbios_put_ch(unsigned char ch)
{
  int col, row;

  __tty_screen.get_cursor(&col, &row);
  vbios_write_ch(ch, &col, &row);
}

/* Write out the character, but don't move the cursor.  */
static void vbios_put_ch_at(unsigned char ch)
{
  __dpmi_regs r = {0};

  r.h.al = ch;
  r.h.bh = __tty_screen.active_page;
  r.x.cx = 1;
  r.h.bl = __tty_screen.attrib;
  r.h.ah = 0x09;
  __dpmi_int(0x10, &r);
}

/* Write out a zero-terminated string of characters.  */
static void vbios_puts(const unsigned char *s)
{
  int col, row;

  __tty_screen.get_cursor(&col, &row);

  while (*s)
  {
    vbios_write_ch(*s, &col, &row);
    ++s;
  }
}

/* Clear from (x1, y1) to (x2, y2).  */
static void vbios_clear(int x1, int y1, int x2, int y2)
{
  __dpmi_regs r = {0};

  /* Clear the first line if just a portion of it is being cleared,
     or if the first line happens to be the last line.  */
  if (x1 > 0 || y1 == y2)
  {
    r.h.ah = 6;
    r.h.al = 0;
    r.h.bh = __tty_screen.attrib;
    r.h.ch = y1;
    r.h.cl = x1;
    r.h.dh = y1;
    r.h.dl = (y1 == y2) ? x2 : __tty_screen.max_col;
    __dpmi_int(0x10, &r);

    ++y1;
    x1 = 0;
  }

  /* The job is finished.  */
  if (y1 > y2)
    return;

  /* If not all of the last line is being cleared, ensure it's dealt with
     seperately.  */
  if (x2 != 0)
    --y2;

  /* Clear the remaining lines except maybe for the last.  */
  if (y1 < y2)
  {
    r.h.ah = 6;
    r.h.al = 0;
    r.h.bh = __tty_screen.attrib;
    r.h.ch = y1;
    r.h.cl = 0;
    r.h.dh = y2;
    r.h.dl = __tty_screen.max_col;
    __dpmi_int(0x10, &r);
  }

  /* Deal with the last line if not all of it is being cleared.  */
  if (x2 != 0 || y1 == y2)
  {
    ++y2;
    r.h.ah = 6;
    r.h.al = 0;
    r.h.bh = __tty_screen.attrib;
    r.h.ch = y1;
    r.h.cl = 0;
    r.h.dh = y2;
    r.h.dl = x2;
    __dpmi_int(0x10, &r);
  }
}

/* Scroll y1 to y2 up to ydst.  */
static void
vbios_scroll_up(int y1, int y2, int ydst)
{
  __dpmi_regs r = {0};
  r.h.ah = 6;
  r.h.al = y1;
  r.h.bh = __tty_screen.attrib;
  r.h.ch = ydst;
  r.h.cl = 0;
  r.h.dh = y2;
  r.h.dl = __tty_screen.max_col;
  __dpmi_int(0x10, &r);
}

/* Scroll y1 to y2 down to ydst.  */
static void
vbios_scroll_down(int y1, int y2, int ydst)
{
  __dpmi_regs r = {0};
  r.h.ah = 7;
  r.h.al = ydst;
  r.h.bh = __tty_screen.attrib;
  r.h.ch = y1;
  r.h.cl = 0;
  r.h.dh = y2;
  r.h.dl = __tty_screen.max_col;
  __dpmi_int(0x10, &r);
}

/* Scroll x1 to x2 right to xdst.  */
static void
vbios_scroll_right(int y, int x1, int x2, int xdst)
{
  int orig_col, orig_row;
  int col_get, col_put;
  __dpmi_regs r_get, r_put;

  __tty_screen.get_cursor(&orig_col, &orig_row);

  col_put = xdst + (x2 - x1);
  col_get = x1 + (x2 - x1);

  if (col_put > __tty_screen.max_col)
  {
    col_get -= (col_put - __tty_screen.max_col);
    col_put = __tty_screen.max_col;
  }

  while (col_get >= x1)
  {
    __tty_screen.set_cursor(col_get, y);

    r_get.h.ah = 8;
    r_get.h.bh = __tty_screen.active_page;
    __dpmi_int(0x10, &r_get);

    __tty_screen.set_cursor(col_put, y);

    r_put.h.ah = 9;
    r_put.h.al = r_get.h.al;
    r_put.h.bh = __tty_screen.active_page;
    r_put.h.bl = r_get.h.ah;
    r_put.x.cx = 1;
    __dpmi_int(0x10, &r_put);

    --col_get;
    --col_put;
  }

  __tty_screen.set_cursor(x1, y);

  r_put.h.ah = 0x09;
  r_put.h.al = ' ';
  r_put.h.bh = __tty_screen.active_page;
  r_put.h.bl = __tty_screen.attrib;
  r_put.x.cx = (col_put - x1 + 1);
  __dpmi_int(0x10, &r_put);

  __tty_screen.set_cursor(orig_col, orig_row);
}

/* Scroll x1 to x2 left to xdst.  */
static void
vbios_scroll_left(int y, int x1, int x2, int xdst)
{
  int orig_col, orig_row;
  int col_get, col_put;
  __dpmi_regs r_get, r_put;
  int scroll_amt;

  __tty_screen.get_cursor(&orig_col, &orig_row);

  col_put = xdst;
  col_get = x1;
  scroll_amt = (x1 - xdst);

  while (col_get <= x2)
  {
    __tty_screen.set_cursor(col_get, y);

    r_get.h.ah = 8;
    r_get.h.bh = __tty_screen.active_page;
    __dpmi_int(0x10, &r_get);

    __tty_screen.set_cursor(col_put, y);

    r_put.h.ah = 9;
    r_put.h.al = r_get.h.al;
    r_put.h.bh = __tty_screen.active_page;
    r_put.h.bl = r_get.h.ah;
    r_put.x.cx = 1;
    __dpmi_int(0x10, &r_put);

    ++col_get;
    ++col_put;
  }

  __tty_screen.set_cursor(col_put, y);

  r_put.h.ah = 0x09;
  r_put.h.al = ' ';
  r_put.h.bh = __tty_screen.active_page;
  r_put.h.bl = __tty_screen.attrib;
  r_put.x.cx = scroll_amt;
  __dpmi_int(0x10, &r_put);

  __tty_screen.set_cursor(orig_col, orig_row);
}
