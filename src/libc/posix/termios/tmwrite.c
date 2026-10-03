/* Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details */
/* Modified from the termios design by Daisuke Aoyama.
   Special thanks to Ryo Shimizu.

   This file writes characters to the screen and interprets output commands
   when the termios emulation is active.

   ECMA-48 commands implemented by Mark E. <snowball3@softhome.net>
   except color support ported from the GNU ls color support
   by Eli Zaretskii.  */

#include <libc/stubs.h>
#include <go32.h>
#include <io.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <libc/file.h>
#include <libc/ttyprvt.h>
#include <libc/farptrgs.h>
#include <libc/getdinfo.h>

#define NPAR 16

struct tty_screen_interface *__tty_screen_intface;

struct tty_screen_info __tty_screen;

/* Console command parser */

enum cmd_parser_states { need_esc = 0, have_esc, have_lbracket,
                         have_arg, have_cmd};

/* Color values. From conio.h.  */
enum COLORS {
    /*  dark colors     */
    BLACK,
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    BROWN,
    LIGHTGRAY,
    /*  light colors    */
    DARKGRAY, /* "light black" */
    LIGHTBLUE,
    LIGHTGREEN,
    LIGHTCYAN,
    LIGHTRED,
    LIGHTMAGENTA,
    YELLOW,
    WHITE
};

/* Static variables used by tmwrite.c  */
static enum cmd_parser_states cmd_state;

static enum COLORS screen_color[] = {BLACK, RED, GREEN, BROWN,
                                     BLUE, MAGENTA, CYAN, LIGHTGRAY};

/* termios tty functions.  */
static ssize_t __libc_termios_write (int handle, const void *buffer, size_t count, ssize_t *rv);
static ssize_t __libc_termios_write_tty(int handle, const void *buffer, size_t count, int cooked);

/* Output command functions.  */
static size_t parse_console_command(const unsigned char *buf, size_t count);
static void execute_console_command(const unsigned char cmd, unsigned char argc, unsigned int args[NPAR]);

/* Screen manipulation functions.  */
static void set_cursor(int col, int row);
static void move_cursor(int x_delta, int y_delta);
static void get_cursor(int *col, int *row);
static void set_cursor_shape(int shape);

/* Attribute helpers.  */
static void set_blink_attrib(int enable_blink);
static void restore_video_state(void);

/* Initialize the screen portion of termios.  */
void __libc_termios_init_write(void)
{
  __dpmi_regs r = {0};
  char *tty_screen;

  /* set special hooks */
  __libc_write_termios_hook = __libc_termios_write;

  /* Initialize the screen variable.  */
  _farsetsel(_dos_ds);
  __tty_screen.active_page = _farnspeekb(0x462);
  __tty_screen.max_row = (int)_farnspeekb(0x484);
  __tty_screen.max_col = (int)_farnspeekw(0x44a) - 1;

  /* Does it normally blink when bg has its 3rd bit set?  */
  __tty_screen.norm_blink = (_farnspeekb(0x465) & 0x20) ? 1 : 0;
  __tty_screen.cur_blink = __tty_screen.norm_blink;

  /* Get initial cursor shape and type.  */
  __tty_screen.init_cursor_shape = _farnspeekw(0x460);

  /* Determine the current attribute.  */
  r.h.ah = 0x08;
  r.h.bh = __tty_screen.active_page;
  __dpmi_int(0x10, &r);

  __tty_screen.init_attrib = r.h.ah;
  __tty_screen.attrib = r.h.ah;

  __tty_screen.set_cursor = set_cursor;
  __tty_screen.get_cursor = get_cursor;

  /* Select between the video bios and the direct video buffer method of
     writing to the screen.  The advantage of the video bios is it usable
     in situations the direct video method isn't.  But when the direct video
     method is usable, it's faster.  */
  tty_screen = getenv("TTY_SCREEN_INTFACE");
  if (tty_screen == NULL || (*tty_screen == 'B' || *tty_screen == 'b'))
    __tty_screen_intface = &__tty_vbios_intface;
  else
    __tty_screen_intface = &__tty_direct_intface;

  /* Allow the driver to perform initialization if it needs to.  */
  if (__tty_screen_intface->init)
    __tty_screen_intface->init();
  atexit(restore_video_state);
}

/******************************************************************************/
/* special write function *****************************************************/

static ssize_t
__libc_termios_write (int handle, const void *buffer, size_t count, ssize_t *rv)
{
  short devmod;
  int raw;

  /* check handle whether valid or not */
  devmod = _get_dev_info (handle);
  if (devmod == -1)
  {
    *rv = -1;
    return 1;
  }

  /* special case */
  if (count == 0)
  {
    *rv = 0;
    return 1;
  }

  /* console only... */
  if ((devmod & _DEV_CDEV) == 0 || (devmod & (_DEV_STDIN|_DEV_STDOUT)) == 0)
    return 0;

  /* Process output? */
  raw = ((devmod & _DEV_RAW) && (__file_handle_modes[handle] & O_BINARY))
        || ((__libc_tty_p->t_oflag & OPOST) == 0);

  *rv = __libc_termios_write_tty(handle, buffer, count, raw);
  return 1;
}

static ssize_t
__libc_termios_write_tty(int handle, const void *buffer, size_t count,
                         int raw)
{
  ssize_t bytes;
  const unsigned char *rp;
  unsigned char ch;
  ssize_t n;
  int col, row;

  bytes = count;

  get_cursor(&col, &row);

  rp = (const unsigned char *)buffer;
  n = count;
  while (n > 0)
  {
    /* get character */
    ch = *rp;

    /* Handle console commands */
    if (ch == '\e' || cmd_state != need_esc)
    {
      size_t delta;

      /* The screen interface implementation of write_ch has the option of
         not updating the cursor position to the current row and column.
         Update the cursor position now to ensure execute_console_command
         gets the right position.  */
      if (ch == '\e')
        set_cursor(col, row);

      delta = parse_console_command(rp, n);

      /* Synchonize in case a command moved the cursor.  */
      get_cursor(&col, &row);

      /* Skip the characters parsed.  */
      n -= delta;
      rp += delta;
      continue;
    }

    ++rp;
    --n;

    /* produce spaces until the next TAB stop */
    if (ch == '\t')
    {
      __tty_screen_intface->write_ch(' ', &col, &row);
      while ((col % 8) != 0)
        __tty_screen_intface->write_ch(' ', &col, &row);
      continue;
    }

    if (!raw)
    {
      /* NOTE: multibyte character don't contain control character */
      /* map NL to CRNL */
      if (ch == '\n' && (__libc_tty_p->t_oflag & ONLCR))
        __tty_screen_intface->write_ch('\r', &col, &row);
      /* map CR to NL */
      else if (ch == '\r' && (__libc_tty_p->t_oflag & OCRNL))
        ch = '\n';
    }
    __tty_screen_intface->write_ch(ch, &col, &row);
  }

  set_cursor(col, row);

  return bytes;
}

/* Characters that can be a command.  */
static inline int
is_command_char(unsigned char ch)
{
  return isalpha(ch) || ch == '@' || ch == '`';
}

/* Parse an output command.  */
static size_t
parse_console_command(const unsigned char *buf, size_t count)
{
  static unsigned int args[NPAR];
  static unsigned char arg_count;
  static unsigned char ignore_cmd;

  const unsigned char *ptr = buf;
  const unsigned char *ptr_end = buf + count;

  while (ptr < ptr_end)
  {
    switch (cmd_state)
    {
      case need_esc:
        /* Find an escape or bail out.  */
        if (*ptr != '\e')
          return (ptr - buf);
        ++ptr;
        cmd_state = have_esc;
        if (ptr >= ptr_end)
          break;

      case have_esc:
        /* Find a left bracket or bail out.  */
        if (*ptr != '[')
        {
          cmd_state = need_esc;
          return (ptr - buf);
        }

        cmd_state = have_lbracket;
        ++ptr;
        if (ptr == ptr_end)
          break;

      case have_lbracket:
        /* After the left bracket, either an argument
           or the command follows.  */
        arg_count = NPAR;
        arg_count = 0;
        args[0] = 0;
        if (isdigit(*ptr))
        {
          cmd_state = have_arg;
        }
        else if (*ptr == '[')
        {
          /* Ignore function keys. */
          ignore_cmd = 1;
          ++ptr;
          break;
        }
        else
        {
          cmd_state = have_cmd;
          break;
        }

      case have_arg:
      {
        /* Parse the argument.  Quit when there are no more digits.  */
        if (isdigit(*ptr))
        {
          do
          {
            args[arg_count] *= 10;
            args[arg_count] += *ptr - '0';
            ++ptr;
          } while (ptr < ptr_end && isdigit(*ptr));
          break;
        }
        else if (*ptr == ';')
        {
          /* Argument separator.  */
          ++arg_count;
          if (arg_count < NPAR)
            args[arg_count] = 0;
          ++ptr;
          break;
        }
        else
        {
          /* End of the current argument.
             Assume the command has been reached.  */
          cmd_state = have_cmd;
          ++arg_count;
        }
      }
      case have_cmd:
      {
        /* Execute the command.  */
        if (is_command_char(*ptr) && !ignore_cmd)
          execute_console_command(*ptr, arg_count, args);

        /* Reset the parse state.  */
        cmd_state = need_esc;
        ignore_cmd = 0;
        ++ptr;
        return ptr - buf;
      }
    }
  }
  return ptr - buf;
}

#define GET_ARG(I, DEFVAL) ((argc > I) ? (args[I]) : (DEFVAL))

/* Execute an output command.  */
static void
execute_console_command(const unsigned char cmd, unsigned char argc,
                        unsigned int args[NPAR])
{
  int col, row;
  int arg, col_arg, row_arg;

  switch (cmd)
  {
    /* Cursor Motion.  */

    case 'A':  /* CUU, VPB: Cursor up.  */
    case 'k':
      move_cursor(0, -GET_ARG(0, 1));
      break;

    case 'B':  /* CUD, VPR: Cursor down.  */
    case 'e':
      move_cursor(0, GET_ARG(0, 1));
      break;

    case 'C':  /* CUF, HPR: Cursor right.  */
    case 'a':
      move_cursor(GET_ARG(0, 1), 0);
      break;

    case 'D':  /* CUB, HPB: Cursor Left.  */
    case 'j':
      move_cursor(-GET_ARG(0, 1), 0);
      break;

    case 'E':  /* CNL: Cursor next line.  */
      get_cursor(&col, &row);
      set_cursor(0, row + GET_ARG(0, 1));
      break;

    case 'F':  /* CPL: Cursor to previous line.  */
      get_cursor(&col, &row);
      set_cursor(0, row - GET_ARG(0, 1));
      break;

    case 'G':  /* CHA: Cursor to column.  */
      get_cursor(&col, &row);
      set_cursor(GET_ARG(0, 1) - 1, row);
      break;

    case 'H':  /* CUP, HVP: Cursor to row and column.  */
    case 'f':
      set_cursor(GET_ARG(1, 1) - 1, GET_ARG(0, 1) - 1);
      break;

    case 'd':  /* VPA: Cursor to row.  */
      get_cursor(&col, &row);
      set_cursor(col, GET_ARG(0, 1) - 1);
      break;

    case 'I':  /* CHT: Cursor to next tab stop.  */
      get_cursor(&col, &row);
      set_cursor(col / 8 * 8 + GET_ARG(0, 1) * 8, row);
      break;

    case 'Z':  /* CBT: Cursor to previous tab stop.  */
      get_cursor(&col, &row);
      if (col > 0 && GET_ARG(0, 1) > 0)
        set_cursor((col - 1) / 8 * 8 - (GET_ARG(0, 1) - 1) * 8, row);
      break;

    /* Edit commands.  */

    case 'J':  /* ED: Erase in Display.  */
      switch (GET_ARG(0, 0))
      {
        case 0:  /* Erase from cursor to end of screen.  */
          get_cursor(&col, &row);
          __tty_screen_intface->clear(col, row, __tty_screen.max_col,
                                               __tty_screen.max_row);
          break;

        case 1:  /* Erase from start of screen to cursor.  */
          get_cursor(&col, &row);
          __tty_screen_intface->clear(0, 0, col, row);
          break;

        case 2:  /* Erase the whole screen.  */
          __tty_screen_intface->clear(0, 0, __tty_screen.max_col,
                                           __tty_screen.max_row);
          break;
      }
      break;

    case 'K':  /* EL: Edit in Line */
      switch (GET_ARG(0, 0))
      {
        case 0:  /* Clear from cursor to end of line.  */
          get_cursor(&col, &row);
          __tty_screen_intface->clear(col, row, __tty_screen.max_col, row);
          break;

        case 1:  /* Clear from start of line to cursor.  */
          get_cursor(&col, &row);
          __tty_screen_intface->clear(0, row, col, row);
          break;

        case 2:  /* Clear the whole line.  */
          get_cursor(&col, &row);
          __tty_screen_intface->clear(0, row, __tty_screen.max_col, row);
          break;
      }
      break;

    case 'L':   /* IL: Insert Line */
      get_cursor(&col, &row);
      row_arg = row + GET_ARG(0, 1);
      if (row_arg <= __tty_screen.max_row)
        __tty_screen_intface->scroll_down(row, __tty_screen.max_row, row_arg);
      else
        __tty_screen_intface->clear(0, row, __tty_screen.max_col,
                                           __tty_screen.max_row);
      break;

    case 'M':  /* DL: Delete Line */
      get_cursor(&col, &row);
      row_arg = GET_ARG(0, 1);
      if (row_arg <= __tty_screen.max_row)
        __tty_screen_intface->scroll_up(row_arg, __tty_screen.max_row, row);
      break;

    case 'S':  /* SU: Scroll Up */
      row_arg = GET_ARG(0, 1);
      if (row_arg <= __tty_screen.max_row)
        __tty_screen_intface->scroll_up(row_arg, __tty_screen.max_row, 0);
      else
        __tty_screen_intface->clear(0, 0, __tty_screen.max_col,
                                          __tty_screen.max_row);
      break;

    case 'T':   /* ST: Scroll Down */
      row_arg = GET_ARG(0, 1);
      if (row_arg <= __tty_screen.max_row)
        __tty_screen_intface->scroll_down(0, __tty_screen.max_row, row_arg);
      else
        __tty_screen_intface->clear(0, 0, __tty_screen.max_col,
                                          __tty_screen.max_row);
      break;

    case '@':  /* ICH: Insert Character */
      get_cursor(&col, &row);
      col_arg = col + GET_ARG(0, 1);
      if (col_arg <= __tty_screen.max_col)
        __tty_screen_intface->scroll_right(row, col, __tty_screen.max_col,
                                           col_arg);
      else
        __tty_screen_intface->clear(col, row, __tty_screen.max_col, row);
      break;


    case 'P':  /* DCH: Delete Character */
      get_cursor(&col, &row);
      col_arg = col + GET_ARG(0, 1);
      if (col_arg <= __tty_screen.max_col)
        __tty_screen_intface->scroll_left(row, col_arg, __tty_screen.max_col,
                                          col);
      else
        __tty_screen_intface->clear(col, row, __tty_screen.max_col, row);
      break;

    case 'X':  /* ECH: Erase Character */
      get_cursor(&col, &row);
      col_arg = col + GET_ARG(0, 1);
      if (col_arg > __tty_screen.max_col)
        col_arg = __tty_screen.max_col;
      __tty_screen_intface->clear(col, row, col_arg,  row);
      break;

    /* Color */

    case 'm':  /* SGR: Set Graphic Rendition */
    {
      int i;
      unsigned char fg, bg;

      if (argc == 0)
      {
        __tty_screen.attrib = __tty_screen.init_attrib;
        break;
      }

      fg = __tty_screen.attrib & 0x0f;
      bg = (__tty_screen.attrib >> 4) & 0x0f;

      i = 0;
      while (i < argc)
      {
        switch(args[i])
        {
          case 0:
            fg = __tty_screen.init_attrib & 0x0f;
            bg = (__tty_screen.init_attrib >> 4) & 0x0f;
            break;

          case 1:	/* intensity on */
            fg |= 8;
            break;

          case 2:	/* intensity off */
            fg &= ~8;

          case 4:	/* underline on.  */
            fg |= 8;	/* We can't, so make it bold instead.  */
            break;

          case 5:	/* blink */
            if (__tty_screen.cur_blink == 0)
            {
              /* Ensure blinking is enabled.  */
              set_blink_attrib(1);
              __tty_screen.cur_blink = 1;
            }
            bg |= 8;
            break;

          case 7:	/* reverse video */
          {
            unsigned char t = fg;
            fg = bg;
            bg = t;

            /* If it was blinking before, let it blink after.  */
            if (fg & 8)
              bg |= 8;

            /* If the fg was bold, let the background be bold.  */
            if ((t & 8) && __tty_screen.cur_blink != 0)
            {
              set_blink_attrib(0);
              __tty_screen.cur_blink = 0;
            }
            break;
          }

          case 8:	/* concealed on */
            fg = (bg & 7) | 8;	/* make fg be like bg, only bright */
            break;

          case 25:	/* no blink */
            if (__tty_screen.cur_blink != 0)
            {
              /* Ensure we aren't in blink mode.  */
              set_blink_attrib(0);
              __tty_screen.cur_blink = 0;
            }
            bg &= ~8;
            break;

          case 30: case 31: case 32: case 33: /* foreground color */
          case 34: case 35: case 36: case 37:
            fg = (fg & 8) | (screen_color[args[i] - 30] & 15);
            break;

          case 40: case 41: case 42: case 43: /* background color */
          case 44: case 45: case 46: case 47:
            bg = (bg & 8) | (screen_color[args[i] - 40] & 15);
            break;

          default:
            break;
        }
        ++i;
      }

     /* They don't *really* want it invisible, do they?  */
     if (fg == (bg & 7))
        fg |= 8;  /* make it concealed instead */

      /* Construct the text attribute and set it.  */
      __tty_screen.attrib = (bg << 4) | fg;
    }
    break;

    case 'v':  /* Change cursor shape.  DJGPP private command.  */
      arg = GET_ARG(0, 0);
      if (arg >= 0 && arg <= 2)
        set_cursor_shape(arg);
      break;
  }
}

/* Get the coordinates of the cursor.  */
static void
get_cursor(int *col, int *row)
{
  unsigned short rowcol;

  rowcol = _farnspeekw(0x450 + __tty_screen.active_page);
  *row = (int)rowcol >> 8;
  *col = (int)(rowcol & 0xff);
}

/* Set the cursor to an absolute position.  */
static void
set_cursor(int col, int row)
{
  __dpmi_regs r = {0};
  unsigned short rowcol;
  int cur_col, cur_row;

  /* Get the current cursor position.  */
  rowcol = _farnspeekw(0x450 + __tty_screen.active_page);
  cur_row = (int)rowcol >> 8;
  cur_col = (int)(rowcol & 0xff);

  /* Range check the new position.  */
  if (row < 0)
    row = 0;
  else if (row > __tty_screen.max_row)
    row = __tty_screen.max_row;

  if (col < 0)
    col = 0;
  else if (col > __tty_screen.max_col)
    col = __tty_screen.max_col;

  /* If the new cursor position is the current position,
     do nothing and avoid the performance hit.  */
  if (col == cur_col && row == cur_row)
    return;

  r.h.ah = 2;
  r.h.bh = __tty_screen.active_page;
  r.h.dh = row;
  r.h.dl = col;
  __dpmi_int(0x10, &r);
}

/* Move the cursor by a relative amount.  */
static void
move_cursor(int x_delta, int y_delta)
{
  int row, col;
  unsigned short rowcol;
  __dpmi_regs r = {0};

  /* Get cursor's current position.  */
  rowcol = _farpeekw(_dos_ds, 0x450 + __tty_screen.active_page);
  row = rowcol >> 8;
  col = rowcol & 0xff;

  /* Sanity check for the row.  */
  if ((int)row + y_delta < 0)
    row = 0;
  else if (row + y_delta > __tty_screen.max_row)
    row = __tty_screen.max_row;

  /* Sanity check for the column.  */
  if ((int)col + x_delta < 0)
    col = 0;
  else if (col + x_delta > __tty_screen.max_col)
    col = __tty_screen.max_col;

  /* Do it.  */
  r.h.ah = 2;
  r.h.bh = __tty_screen.active_page;
  r.h.dh = row + y_delta;
  r.h.dl = col + x_delta;
  __dpmi_int(0x10, &r);
}

#define NORMAL_CURSOR    0
#define INVISIBLE_CURSOR 1
#define ENHANCED_CURSOR  2

static void
set_cursor_shape(int shape)
{
  __dpmi_regs r = {0};
  unsigned short max_line, bot_line, top_line;
  unsigned short new_cursor;

  if (shape == INVISIBLE_CURSOR)
  {
    new_cursor = 0x2000;
  }
  else if (shape == NORMAL_CURSOR)
  {
    new_cursor = __tty_screen.init_cursor_shape;
  }
  else
  {
    max_line = _farpeekw(_dos_ds, 0x0485) - 1;
    switch (max_line)
    {
      default:
        max_line = 7;
      case 7:
      case 9:
        bot_line = max_line;
        break;

      case 13:
      case 15:
        bot_line = max_line - 1;
        break;
    }
    if (shape == ENHANCED_CURSOR)
      top_line = 0;
    else
      top_line = bot_line - 1;

    new_cursor = ((top_line & 0x1f) << 8) | (bot_line & 0x1f);
  }

  if (new_cursor == _farpeekw(_dos_ds, 0x460))
    return;

  r.x.cx = new_cursor;
  r.h.ah = 1;
  __dpmi_int(0x10, &r);
}

/* Blink support functions.  */

static void
set_blink_attrib(int enable_blink)
{
  __dpmi_regs r = {0};

  r.h.bh = 0;
  r.h.bl = (enable_blink) ? 1 : 0;
  r.x.ax = 0x1003;
  __dpmi_int(0x10, &r);
}

/* Restore the BIOS blinking bit to its original value.  Called at exit.  */
static void
restore_video_state(void)
{
  __dpmi_regs r = {0};

  if (__tty_screen.cur_blink != __tty_screen.norm_blink)
    set_blink_attrib(__tty_screen.norm_blink);

  if (__tty_screen.init_cursor_shape != _farpeekw(_dos_ds, 0x460))
  {
    r.h.ah = 1;
    r.x.cx = __tty_screen.init_cursor_shape;
    __dpmi_int(0x10, &r);
  }
}

