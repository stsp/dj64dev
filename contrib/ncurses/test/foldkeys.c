/****************************************************************************
 * Copyright 2018-2024,2025 Thomas E. Dickey                                *
 * Copyright 2006-2016,2017 Free Software Foundation, Inc.                  *
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
 * Author: Thomas E. Dickey, 2006
 *
 * $Id: foldkeys.c,v 1.16 2025/07/05 15:11:35 tom Exp $
 *
 * Demonstrate a method for altering key definitions at runtime.
 *
 * This program reads the key definitions, merging those which have xterm-style
 * modifiers into their equivalents which have no modifiers.  It does this
 * merging only for the keys which are defined in the terminal description.
 */

#define NEED_TIME_H
#include <test.priv.h>

#if defined(NCURSES_VERSION) && NCURSES_EXT_FUNCS

#define MY_LOGFILE "demo_foldkeys.log"
#define MY_KEYS (KEY_MAX + 1)

/*
 * Log the most recently-written line to our logfile
 */
static void
log_last_line(WINDOW *win)
{
    FILE *fp;

    if ((fp = fopen(MY_LOGFILE, "a")) != NULL) {
	char temp[256];
	int y, x, n;
	int need = sizeof(temp) - 1;
	if (need > COLS)
	    need = COLS;
	getyx(win, y, x);
	wmove(win, y - 1, 0);
	n = winnstr(win, temp, need);
	while (n-- > 0) {
	    if (isspace(UChar(temp[n])))
		temp[n] = '\0';
	    else
		break;
	}
	wmove(win, y, x);
	fprintf(fp, "%s\n", temp);
	fclose(fp);
    }
}

/*
 * ncurses has no API for telling what the actual last key-code is.  That is
 * a secret because the codes past KEY_MAX are computed at run-time and may
 * differ depending on the previous calls to newterm(), etc.  It is unlikely
 * that one could have more than a thousand key definitions...
 */
#define MAX_KEYS 2000

typedef struct {
    const char *name;
    const char *value;
    int code;
    int state;
} KeyInfo;

static void
demo_foldkeys(void)
{
    KeyInfo info[MAX_KEYS];
    int info_len = 0;
    int merged = 0;
    int code;
    int j, k;

    /*
     * Tell ncurses that we want to use function keys.  That will make it add
     * any user-defined keys that appear in the terminfo.
     */
    keypad(stdscr, TRUE);

    /*
     * List the predefined keys using the strnames[] array.
     */
    for (code = 0; code < STRCOUNT; ++code) {
	NCURSES_CONST char *name = strnames[code];
	NCURSES_CONST char *value = tigetstr(name);
	if (value != NULL && value != (NCURSES_CONST char *) -1) {
	    info[info_len].name = strnames[code];
	    info[info_len].code = key_defined(value);
	    info[info_len].value = value;
	    info[info_len].state = 0;
	    if (info[info_len].code > 0)
		++info_len;
	}
    }

    /*
     * We can get the names for user-defined keys from keyname().  It returns
     * a name like KEY_foo for the predefined keys, which tigetstr() does not
     * understand.
     */
    for (code = KEY_MAX; code < MAX_KEYS; ++code) {
	NCURSES_CONST char *name = keyname(code);
	if (name != NULL) {
	    info[info_len].name = name;
	    info[info_len].code = code;
	    info[info_len].value = tigetstr(name);
	    info[info_len].state = 0;
	    ++info_len;
	}
    }
    printw("Initially %d key definitions\n", info_len);

    /*
     * Look for keys that have xterm-style modifiers.
     */
    for (j = 0; j < info_len; ++j) {
	int first, second;
	char final[2];
	char *value;
	size_t need;

	if (info[j].state == 0
	    && sscanf(info[j].value,
		      "\033[%d;%d%c",
		      &first,
		      &second,
		      final) == 3
	    && *final != ';'
	    && first >= 0
	    && first < 1024
	    && (need = strlen(info[j].value)) != 0
	    && (value = malloc(need + 8)) != NULL) {
	    (void) need;	/* _nc_SLIMIT is normally nothing  */
	    _nc_SPRINTF(value, _nc_SLIMIT(need) "\033[%d%c", first, *final);
	    for (k = 0; k < info_len; ++k) {
		if (info[k].state == 0
		    && !strcmp(info[k].value, value)) {
		    info[j].state = 1;
		    break;
		}
	    }
	    if (info[j].state == 0) {
		_nc_SPRINTF(value, _nc_SLIMIT(need) "\033O%c", *final);
		for (k = 0; k < info_len; ++k) {
		    if (info[k].state == 0
			&& !strcmp(info[k].value, value)) {
			info[j].state = 1;
			break;
		    }
		}
	    }
	    if (info[j].state == 1) {
		if ((define_key(info[j].value, info[k].code)) != ERR) {
		    printw("map %s to %s\n", info[j].value, info[k].value);
		    keyok(info[j].code, FALSE);
		    ++merged;
		} else {
		    printw("? cannot define_key %d:%s\n", j, info[j].value);
		}
	    } else {
		printw("? cannot merge %d:%s\n", j, info[j].value);
	    }
	    free(value);
	}
    }
    printw("Merged to %d key definitions\n", info_len - merged);
}

static void
usage(int ok)
{
    static const char *msg[] =
    {
	"Usage: foldkeys [options]"
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
    TimeType previous;

    while ((ch = getopt(argc, argv, OPTS_COMMON)) != -1) {
	switch (ch) {
	default:
	    CASE_COMMON;
	    /* NOTREACHED */
	}
    }
    if (optind < argc)
	usage(FALSE);

    if (newterm(NULL, stdout, stdin) == NULL) {
	fprintf(stderr, "Cannot initialize terminal\n");
	ExitProgram(EXIT_FAILURE);
    }

    unlink(MY_LOGFILE);

    (void) cbreak();		/* take input chars one at a time, no wait for \n */
    (void) noecho();		/* don't echo input */

    scrollok(stdscr, TRUE);
    keypad(stdscr, TRUE);
    move(0, 0);

    demo_foldkeys();

    GetClockTime(&previous);

    while ((ch = getch()) != ERR) {
	bool escaped = (ch >= MY_KEYS);
	const char *name = keyname(escaped ? (ch - MY_KEYS) : ch);
	TimeType current;

	GetClockTime(&current);
	printw("%6.03f ", ElapsedSeconds(&previous, &current));
	previous = current;

	printw("Keycode %d, name %s%s\n",
	       ch,
	       escaped ? "ESC-" : "",
	       name != NULL ? name : "<null>");
	log_last_line(stdscr);
	clrtoeol();
	if (ch == 'q')
	    break;
    }
    endwin();
    ExitProgram(EXIT_SUCCESS);
}
#else
int
main(void)
{
    printf("This program requires the ncurses library\n");
    ExitProgram(EXIT_FAILURE);
}
#endif
