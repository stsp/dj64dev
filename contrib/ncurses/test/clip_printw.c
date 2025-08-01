/****************************************************************************
 * Copyright 2019-2024,2025 Thomas E. Dickey                                *
 * Copyright 2008-2016,2017 Free Software Foundation, Inc.                  *
 *                                                                          *
 * Permission is hereby granted, free of charge, to any person obtaining a  *
 * copy of this software and associated documentation files (the            *
 * "Software"), to deal in the Software without restriction, including      *
 * without limitation the rights to use, copy, modify, merge, publish,      *
 * distribute, distribute with modifications, sublicense, and/or sell       *
 * copies of the Software, and to permit persons to whom the Software is    *
 * furnished to do so, subject to the following conditions:                 *
 *                                                                          *
 * The above copyright notice and this permission notice shall be included  *
 * in all copies or substantial portions of the Software.                   *
 *                                                                          *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS  *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF               *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   *
 * IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR    *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR    *
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 *                                                                          *
 * Except as contained in this notice, the name(s) of the above copyright   *
 * holders shall not be used in advertising or otherwise to promote the     *
 * sale, use or other dealings in this Software without prior written       *
 * authorization.                                                           *
 ****************************************************************************/
/*
 * $Id: clip_printw.c,v 1.27 2025/07/05 15:21:56 tom Exp $
 *
 * demonstrate how to use printw with/without wrapping.
 */

#include <test.priv.h>
#include <popup_msg.h>

#ifdef HAVE_VW_PRINTW

#define SHOW(n) ((n) == ERR ? "ERR" : "OK")
#define COLOR_DEFAULT (-1)

#define X_BASE 0
#define Y_BASE 1
#define MARGIN 1
#define Y_TOP  (Y_BASE + MARGIN)

typedef struct {
    unsigned c;
    unsigned v;
    bool single;
    int status;
    int pair;
    attr_t attr;
    int count;
    int ch;
    const char *c_msg;
    const char *v_msg;
    int y_val;
    int x_val;
    int y_beg, x_beg;
    int y_max, x_max;
} STATUS;

static int
clip_wprintw(WINDOW *win, bool single, NCURSES_CONST char *fmt, ...)
{
    int y0, x0, y1, x1, width;
    WINDOW *sub;
    va_list ap;
    int rc;

    getyx(win, y0, x0);
    width = getmaxx(win) - x0;
    if (single) {
	/*
	 * Allocate a single-line derived window extending from the current
	 * cursor position to the end of the current line in the given window.
	 * Disable scrolling in the derived window.
	 */
	sub = derwin(win, 1, width, y0, x0);
    } else {
	/*
	 * Allow printw to wrap through the entire window.
	 */
	sub = derwin(win, getmaxy(win), getmaxx(win), 0, 0);
	wmove(sub, y0, x0);
    }
    scrollok(sub, FALSE);

    /*
     * Print the text.
     */
    va_start(ap, fmt);
    rc = vw_printw(sub, fmt, ap);
    va_end(ap);

    getyx(sub, y1, x1);
    delwin(sub);

    if (single) {
	wmove(win, y1 + y0, x1 + x0);
    } else {
	wmove(win, y1, x1);
    }

    return rc;
}

static const char *
color_params(unsigned state, int *pair)
{
    /* *INDENT-OFF* */
    static struct {
	int pair;
	int fg, bg;
	const char *msg;
    } table[] = {
	{ 0, COLOR_DEFAULT, COLOR_DEFAULT, "default" },
	{ 1, COLOR_RED,     COLOR_BLACK,   "red/black" },
	{ 2, COLOR_WHITE,   COLOR_BLUE,    "white/blue" },
    };
    /* *INDENT-ON* */

    const char *result = NULL;

    if (has_colors()) {
	static bool first = TRUE;

	if (first) {
	    unsigned n;

	    start_color();
	    for (n = 0; n < SIZEOF(table); ++n) {
		init_pair((short) table[n].pair,
			  (short) table[n].fg,
			  (short) table[n].bg);
	    }
	}
	if (state < SIZEOF(table)) {
	    *pair = table[state].pair;
	    result = table[state].msg;
	}
    }
    return result;
}

static const char *
video_params(unsigned state, attr_t *attr)
{
    /* *INDENT-OFF* */
    static struct {
	attr_t attr;
	const char *msg;
    } table[] = {
	{ WA_NORMAL,	"normal" },
	{ WA_BOLD,	"bold" },
	{ WA_REVERSE,	"reverse" },
	{ WA_UNDERLINE,	"underline" },
	{ WA_BLINK, 	"blink" },
    };
    /* *INDENT-ON* */

    const char *result = NULL;

    if (state < SIZEOF(table)) {
	*attr = table[state].attr;
	result = table[state].msg;
    }
    return result;
}

/* fill the window with a test-pattern */
static void
fill_window(WINDOW *win)
{
    int y_top = (win == stdscr) ? Y_BASE : MARGIN;
    int y, x;
    int y0 = -1, x0 = -1;

    getyx(win, y, x);
    wmove(win, y_top, 0);
    while (waddstr(win, "0123456789 abcdefghijklmnopqrstuvwxyz ") != ERR) {
	int y1, x1;
	getyx(win, y1, x1);
	if (y1 == y0 && x1 == x0)
	    break;
	x0 = x1;
	y0 = y1;
    }
    wmove(win, y, x);
}

static void
show_status(WINDOW *win, STATUS * sp)
{
    int y, x;

    getyx(win, y, x);
    wattron(win, A_REVERSE);
    wmove(win, 0, 0);
    wprintw(win, "Clip %s", sp->single ? "line" : "window");
    wprintw(win, " Count %d", sp->count);
    if (sp->v_msg != NULL)
	wprintw(win, " Video %s", sp->v_msg);
    if (sp->c_msg != NULL)
	wprintw(win, " Color %s", sp->c_msg);
    wprintw(win, " (%d)", sp->status);
    wclrtoeol(win);
    wattroff(win, A_REVERSE);
    wmove(win, y, x);
}

static void
do_subwindow(WINDOW *win, const STATUS * sp, void func(WINDOW *))
{
    WINDOW *win1 = newwin(sp->y_max - (2 * MARGIN),
			  sp->x_max - (2 * MARGIN),
			  sp->y_beg + MARGIN,
			  sp->x_beg + MARGIN);

    if (win1 != NULL && sp->y_max > 4 && sp->x_max > 4) {
	WINDOW *win2 = derwin(win1,
			      sp->y_max - (2 * MARGIN) - 2,
			      sp->x_max - (2 * MARGIN) - 2,
			      (win == stdscr) ? Y_BASE : Y_BASE,
			      MARGIN);

	if (win2 != NULL) {
	    box(win1, 0, 0);
	    wrefresh(win1);
	    func(win2);

	    delwin(win2);
	} else {
	    beep();
	}
	delwin(win1);
	touchwin(win);
    } else {
	if (win1)
	    delwin(win1);
	beep();
    }
}

/*
 * The status line is within the same window as the test-data.
 */
static void
init_status(WINDOW *win, STATUS * sp)
{
    memset(sp, 0, sizeof(*sp));
    sp->single = TRUE;
    sp->c = 99;
    sp->v = 99;
    sp->ch = ' ';

    keypad(win, TRUE);
    fill_window(win);

    getbegyx(win, sp->y_beg, sp->x_beg);
    getmaxyx(win, sp->y_max, sp->x_max);

    wmove(win, sp->y_val = Y_BASE, sp->x_val = 0);
}

static void
show_help(WINDOW *win)
{
    static NCURSES_CONST char *msgs[] =
    {
	"Basic commands:"
	,"Use h/j/k/l or arrow keys to move the cursor."
	,"Set the count parameter for clip_wprintw by entering digits 0-9."
	,""
	,"Other commands:"
	,"space toggles through the set of video attributes and colors."
	,"q     exit subwindow, quit if last window."
	,"s     toggles single-line updates versus entire windows."
	,"t     touches (forces repaint) of the current line."
	,"w     create subwindow."
	,".     calls vw_printw at the current position with the given count."
	,"=     resets count to zero."
	,"?     shows this help-window"
	,NULL
    };

    popup_msg(win, msgs);
}

static void
update_status(WINDOW *win, STATUS * sp)
{
    int margin = 0;

    switch (sp->ch) {
    case ' ':			/* next test-iteration */
	if (has_colors()) {
	    if ((sp->c_msg = color_params(++(sp->c), &(sp->pair))) == NULL) {
		sp->c_msg = color_params(sp->c = 0, &(sp->pair));
		if ((sp->v_msg = video_params(++(sp->v), &(sp->attr))) == NULL) {
		    sp->v_msg = video_params(sp->v = 0, &(sp->attr));
		}
	    }
	} else {
	    if ((sp->v_msg = video_params(++(sp->v), &(sp->attr))) == NULL) {
		sp->v_msg = video_params(sp->v = 0, &(sp->attr));
	    }
	}
	sp->count = 0;
	show_status(win, sp);
	break;
    case KEY_HOME:
    case '^':
	wmove(win, sp->y_val, sp->x_val = margin);
	break;
    case KEY_END:
    case '$':
	wmove(win, sp->y_val, sp->x_val = (sp->x_max - margin - 1));
	break;
    case KEY_PPAGE:
    case CTRL('B'):
	wmove(win, sp->y_val = Y_BASE, sp->x_val);
	break;
    case KEY_NPAGE:
    case CTRL('F'):
	wmove(win, sp->y_val = (sp->y_max - margin - 1), sp->x_val);
	break;
    case KEY_LEFT:
    case 'h':
	if (sp->x_val > margin)
	    wmove(win, sp->y_val, --(sp->x_val));
	break;
    case KEY_DOWN:
    case 'j':
	if (sp->y_val < (sp->y_max - margin))
	    wmove(win, ++(sp->y_val), sp->x_val);
	break;
    case KEY_UP:
    case 'k':
	if (sp->y_val > Y_BASE)
	    wmove(win, --(sp->y_val), sp->x_val);
	break;
    case KEY_RIGHT:
    case 'l':
	if (sp->x_val < (sp->x_max - margin))
	    wmove(win, sp->y_val, ++(sp->x_val));
	break;
    case 't':
	touchline(win, sp->y_val, 1);
	break;
    case '=':
	sp->count = 0;
	show_status(win, sp);
	break;
    case 's':
	sp->single = !sp->single;
	show_status(win, sp);
	break;
    case HELP_KEY_1:
	show_help(win);
	break;
    default:
	if (isdigit(sp->ch)) {
	    sp->count = (sp->count * 10) + (sp->ch - '0');
	    show_status(win, sp);
	} else {
	    beep();
	}
	break;
    }
}

static void
test_clipping(WINDOW *win)
{
    STATUS st;
    char fmt[80];
    char *buffer;
    unsigned j, need;

    init_status(win, &st);

    do {
	switch (st.ch) {
	case '.':		/* change from current position */
	    (void) wattrset(win, AttrArg(COLOR_PAIR(st.pair), st.attr));
	    if (st.count > 0) {
		need = (unsigned) st.count + 1;
		_nc_SPRINTF(fmt, _nc_SLIMIT(sizeof(fmt)) "%%c%%%ds%%c", st.count);
	    } else {
		int want = getmaxx(win);
		if (want < 10)
		    want = 10;
		need = (unsigned) want - 1;
		_nc_STRCPY(fmt, "%c%s%c", sizeof(fmt));
	    }
	    if ((buffer = typeMalloc(char, need + 1)) != NULL) {
		for (j = 0; j < need; ++j) {
		    buffer[j] = (char) ('A' + (j % 26));
		}
		buffer[need - 1] = '\0';
		st.status = clip_wprintw(win, st.single, fmt, '[', buffer, ']');
		free(buffer);
	    }
	    break;
	case 'w':
	    do_subwindow(win, &st, test_clipping);
	    break;
	case 'q':
	    return;
	default:
	    update_status(win, &st);
	    break;
	}
    } while ((st.ch = wgetch(win)) != ERR);
}

static void
usage(int ok)
{
    static const char *msg[] =
    {
	"Usage: clip_printw [options]"
	,""
	,USAGE_COMMON
    };
    size_t n;

    for (n = 0; n < SIZEOF(msg); n++)
	fprintf(stderr, "%s\n", msg[n]);

    ExitProgram(ok ? EXIT_SUCCESS : EXIT_FAILURE);
}
/* *INDENT-OFF* */
VERSION_COMMON()
/* *INDENT-ON* */

int
main(int argc, char *argv[])
{
    int ch;

    while ((ch = getopt(argc, argv, OPTS_COMMON)) != -1) {
	switch (ch) {
	default:
	    CASE_COMMON;
	    /* NOTREACHED */
	}
    }
    if (optind < argc)
	usage(FALSE);

    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();

    test_clipping(stdscr);
    endwin();

    ExitProgram(EXIT_SUCCESS);
}

#else
int
main(void)
{
    printf("This program requires the curses vw_printw function\n");
    ExitProgram(EXIT_FAILURE);
}
#endif
