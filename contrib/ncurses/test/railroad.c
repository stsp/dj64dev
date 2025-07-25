/****************************************************************************
 * Copyright 2019-2024,2025 Thomas E. Dickey                                *
 * Copyright 2000-2013,2017 Free Software Foundation, Inc.                  *
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
 * Author: Thomas E. Dickey - 2000
 *
 * $Id: railroad.c,v 1.29 2025/07/05 15:21:56 tom Exp $
 *
 * A simple demo of the termcap interface.
 */
#define USE_TINFO
#include <test.priv.h>

#if HAVE_TGETENT

static char *wipeit;
static char *moveit;
static int length;
static int height;

static char *finisC;
static char *finisS;
static char *finisU;

static char *startC;
static char *startS;
static char *startU;

static char *backup;

static bool interrupted = FALSE;

static
TPUTS_PROTO(outc, c)
{
    int rc = OK;

    if (interrupted) {
	char tmp = (char) c;
	if (write(STDOUT_FILENO, &tmp, (size_t) 1) == -1)
	    rc = ERR;
    } else {
	if (putc(c, stdout) == EOF)
	    rc = ERR;
    }
    TPUTS_RETURN(rc);
}

static void
PutChar(int ch)
{
    putchar(ch);
    fflush(stdout);
    napms(moveit ? 10 : 50);	/* not really termcap... */
}

static void
Backup(void)
{
    tputs(backup != NULL ? backup : "\b", 1, outc);
}

static void
MyShowCursor(int flag)
{
    if (startC != NULL && finisC != NULL) {
	tputs(flag ? startC : finisC, 1, outc);
    }
}

static void
StandOut(int flag)
{
    if (startS != NULL && finisS != NULL) {
	tputs(flag ? startS : finisS, 1, outc);
    }
}

static void
Underline(int flag)
{
    if (startU != NULL && finisU != NULL) {
	tputs(flag ? startU : finisU, 1, outc);
    }
}

static void
ShowSign(char *string)
{
    const char *base = string;
    int first, last;

    if (moveit != NULL) {
	tputs(tgoto(moveit, 0, height - 1), 1, outc);
	tputs(wipeit, 1, outc);
    }

    while (*string != 0) {
	int ch = *string;
	if (ch != ' ') {
	    if (moveit != NULL) {
		for (first = length - 2; first >= (string - base); first--) {
		    if (first < length - 1) {
			tputs(tgoto(moveit, first + 1, height - 1), 1, outc);
			PutChar(' ');
		    }
		    tputs(tgoto(moveit, first, height - 1), 1, outc);
		    PutChar(ch);
		}
	    } else {
		last = ch;
		if (isalpha(ch)) {
		    first = isupper(ch) ? 'A' : 'a';
		} else if (isdigit(ch)) {
		    first = '0';
		} else {
		    first = ch;
		}
		if (first < last) {
		    Underline(1);
		    while (first < last) {
			PutChar(first);
			Backup();
			first++;
		    }
		    Underline(0);
		}
	    }
	    if (moveit != NULL)
		Backup();
	}
	StandOut(1);
	PutChar(ch);
	StandOut(0);
	fflush(stdout);
	string++;
    }
    if (moveit != NULL)
	tputs(wipeit, 1, outc);
    putchar('\n');
}

static void
cleanup(void)
{
    Underline(0);
    StandOut(0);
    MyShowCursor(1);
}

static void
onsig(int n GCC_UNUSED)
{
    interrupted = TRUE;
    cleanup();
    ExitProgram(EXIT_FAILURE);
}

static void
railroad(char **args)
{
    NCURSES_CONST char *name = getenv("TERM");
    char buffer[1024];
    char area[1024], *ap = area;
    int z;

    if (name == NULL)
#ifdef EXP_WIN32_DRIVER
	name = "ms-terminal";
#else
	name = "dumb";
#endif

    InitAndCatch(z = tgetent(buffer, name), onsig);
    if (z >= 0) {

	wipeit = tgetstr("ce", &ap);
	height = tgetnum("li");
	length = tgetnum("co");
	moveit = tgetstr("cm", &ap);

	if (wipeit == NULL
	    || moveit == NULL
	    || height <= 0
	    || length <= 0) {
	    wipeit = NULL;
	    moveit = NULL;
	    height = 0;
	    length = 0;
	}

	startS = tgetstr("so", &ap);
	finisS = tgetstr("se", &ap);

	startU = tgetstr("us", &ap);
	finisU = tgetstr("ue", &ap);

	backup = tgetstr("le", &ap);

	startC = tgetstr("ve", &ap);
	finisC = tgetstr("vi", &ap);

	MyShowCursor(0);

	while (*args) {
	    ShowSign(*args++);
	}
	MyShowCursor(1);
    }
}

static void
usage(int ok)
{
    static const char *msg[] =
    {
	"Usage: railroad [options]"
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

    if (optind < argc) {
	railroad(argv + optind);
    } else {
	static char world[] = "Hello World";
	static char *hello[] =
	{world, NULL};
	railroad(hello);
    }
    ExitProgram(EXIT_SUCCESS);
}

#else
int
main(void)
{
    printf("This program requires termcap\n");
    exit(EXIT_FAILURE);
}
#endif
