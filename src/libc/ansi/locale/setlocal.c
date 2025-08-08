/*
 * setlocal.c: Set or read international environment
 *
 * Copyright (C) 2002, 2005 Alexander S. Aganichev <asa@users.sf.net>
 *
 * This software may be used freely so long as this copyright notice is
 * left intact.  There is no warranty on this software.
 */

#include <locale.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <dpmi.h>
#ifdef TEST
#include <sys/farptr.h>
#else
#include <libc/farptrgs.h>
#endif
#include <go32.h>
#include <limits.h>

/*
 * Number of supported categories
 */
#define LC_CATEGORIES 5

/*
 * Maximum name length for locale
 */
#define LC_MAXNAMESIZE 16

/*
 * Buffer size for the defined locales
 */
#define LC_BUFSIZ (LC_MAXNAMESIZE * LC_CATEGORIES)

/*
 * This variable contains the locales defined in the order LC_COLLATE,
 * LC_CTYPE, LC_MONETARY, LC_NUMERIC, LC_TIME separated by comma. If all
 * locales are the same, then locale specified only once.
 */
static char lc_buffer[LC_BUFSIZ];
static char lc_current[LC_CATEGORIES][LC_MAXNAMESIZE] =
{
  "C", "C", "C", "C", "C"
};

/*
 * This is what we can extract from country.sys for our purposes.
 */
static char currency_symbol[6] = "";
static char mon_decimal_point[2] = "";
static char mon_thousands_sep[2] = "";
static char decimal_point[2] = ".";
static char thousands_sep[2] = "";

extern unsigned char __dj_collate_table[];
extern char __dj_date_format[];
extern char __dj_time_format[];

/*
 * Reset collate table to the C locale
 */
static int
resetlocalecollate(void)
{
  int i;

  for (i = 0; i < 256; i++)
    __dj_collate_table[i] = i;
  return 1;
}

/*
 * Reset ctype tables to the C locale
 */
static int
resetlocalectype(void)
{
  int i;

  /* ASCII portion always left unchanged */
  for (i = 128; i < 256; i++)
  {
    __dj_ctype_tolower[i + 1] = i;
    __dj_ctype_toupper[i + 1] = i;
    __dj_ctype_flags[i + 1] = 0;
  }
  return 1;
}

/*
 * Reset monetary to the C locale
 * lcnv should be non-NULL
 */
static int
resetlocalemonetary(void)
{
  int honored = 0;
  struct lconv *lcnv = localeconv();

  if(lcnv != NULL)
  {
    *currency_symbol = '\0';
    lcnv->int_curr_symbol = lcnv->currency_symbol = currency_symbol;
    *mon_thousands_sep = '\0';
    lcnv->mon_thousands_sep = mon_thousands_sep;
    *mon_decimal_point = '\0';
    lcnv->mon_decimal_point = mon_decimal_point;
    /* lcnv->mon_grouping = ""; */
    /* lcnv->negative_sign = ""; */
    /* lcnv->positive_sign = ""; */
    lcnv->int_frac_digits = lcnv->frac_digits = CHAR_MAX;
    lcnv->p_cs_precedes = lcnv->n_cs_precedes = CHAR_MAX;
    lcnv->p_sep_by_space = lcnv->n_sep_by_space = CHAR_MAX;
    /* lcnv->p_sign_posn = lcnv->n_sign_posn = CHAR_MAX; */
    honored = 1;
  }
  return honored;
}

/*
 * Reset numeric to the C locale
 * lcnv should be non-NULL
 */
static int
resetlocalenumeric(void)
{
  int honored = 0;
  struct lconv *lcnv = localeconv();

  if(lcnv != NULL)
  {
    *thousands_sep = '\0';
    lcnv->thousands_sep = thousands_sep;
    strcpy(decimal_point, ".");
    lcnv->decimal_point = decimal_point;
    /* lcnv->grouping = ""; */
    honored = 1;
  }
  return honored;
}

/*
 * Reset time strings to the C locale
 */
static int
resetlocaletime(void)
{
  strcpy(__dj_date_format, "%m/%d/%y");
  strcpy(__dj_time_format, "%H:%M:%S");
  return 1;
}

/*
 * Set collate table to the locale specified
 * regs->x.bx = code page
 * regs->x.dx = country ID
 * regs->x.es/di = segment/offset
 */
static int
setlocalecollate(const char *locale __attribute__((unused)), int selector,
                 __dpmi_regs *regs)
{
  regs->h.ah = 0x65;
  regs->h.al = 0x06;
  regs->x.cx = 5;
  __dpmi_int(0x21, regs);
  if ((regs->x.flags & 1) || (regs->x.cx != 5))
    return 0;
  else
  {
    unsigned int table = _farpeekw(selector, 3) * 16 + _farpeekw(selector, 1);
    int size = _farpeekw(_dos_ds, table);

    fmemcpy2(__dj_collate_table, DP(_dos_ds, table + 2), size);
    return 1;
  }
}

/*
 * Set ctype table to the locale specified
 * regs->x.bx = code page
 * regs->x.dx = country ID
 * regs->x.es/di = segment/offset
 */
static int
setlocalectype(const char *locale __attribute__((unused)), int selector,
               __dpmi_regs *regs)
{
  int temp_flags;
  int i;

  regs->h.ah = 0x65;
  regs->h.al = 0x02;
  regs->x.cx = 5;
  __dpmi_int(0x21, regs);
  if ((regs->x.flags & 1) || (regs->x.cx != 5))
    return 0;
  else
  {
    unsigned int table = _farpeekw(selector, 3) * 16 + _farpeekw(selector, 1);
    int size = _farpeekw(_dos_ds, table);

    fmemcpy2(&(__dj_ctype_toupper[128 + 1]), DP(_dos_ds, table + 2), size);

    /* let's build lowercase table from uppercase... */
    for (i = 0; i < size; i++)
    {
      int c = toupper(i + 128);
      if ((c != i + 128) && (c > 127))
        __dj_ctype_tolower[c + 1] = i + 128;
    }
    for (i = 128; i < 256; i++)
    {
      /*
       * Actually isgraph(), ispunct() and isspace() will return wrong results
       * for some letters like 0xff in CP866 but we can't detect them reliably
       */
      temp_flags = __dj_ISPRINT | __dj_ISGRAPH;
      if (tolower(i) != toupper(i))
      {
        temp_flags |= __dj_ISALPHA | __dj_ISALNUM;
        if (i == toupper(i))
          temp_flags |= __dj_ISUPPER;
        else
          temp_flags |= __dj_ISLOWER;
      }
      else
        temp_flags |= __dj_ISPUNCT;
      __dj_ctype_flags[i + 1] = temp_flags;
    }
    return 1;
  }
}

/*
 * Set monetary values to the locale specified
 */
static int
setlocalemonetary(const char *locale, int selector,
                  __dpmi_regs *regs __attribute__((unused)))
{
  struct lconv *lcnv = localeconv();

  if(lcnv == NULL)
    return 0;
  else
  {
    /* parse: de_DE_EURO.850 */
    const char *p = strrchr(locale, '_');

    if ((p != NULL) && !strnicmp(p + 1, "EURO", 4))
      strcpy(currency_symbol, "EUR");
    else
      fmemcpy2(currency_symbol, DP(selector, 9), 5);
    lcnv->int_curr_symbol = lcnv->currency_symbol = currency_symbol;
    fmemcpy2(mon_thousands_sep, DP(selector, 14), 2);
    lcnv->mon_thousands_sep = mon_thousands_sep;
    fmemcpy2(mon_decimal_point, DP(selector, 16), 2);
    lcnv->mon_decimal_point = mon_decimal_point;
    /* lcnv->mon_grouping = ""; */
    /* lcnv->negative_sign = ""; */
    /* lcnv->positive_sign = ""; */
    lcnv->int_frac_digits = lcnv->frac_digits = _farpeekb(selector, 24);
    lcnv->p_cs_precedes = lcnv->n_cs_precedes = _farpeekb(selector, 23) & 1;
    lcnv->p_sep_by_space = lcnv->n_sep_by_space = _farpeekb(selector, 23) & 2;
    /* lcnv->p_sign_posn = lcnv->n_sign_posn = CHAR_MAX; */
    return 1;
  }
}

/*
 * Set numeric values to the locale specified
 */
static int
setlocalenumeric(const char *locale __attribute__((unused)), int selector,
                 __dpmi_regs *regs __attribute__((unused)))
{
  struct lconv *lcnv = localeconv();

  if(lcnv == NULL)
    return 0;
  else
  {
    fmemcpy2(thousands_sep, DP(selector, 14), 2);
    lcnv->thousands_sep = thousands_sep;
    fmemcpy2(decimal_point, DP(selector, 16), 2);
    lcnv->decimal_point = decimal_point;
    /* lcnv->grouping = ""; */
    return 1;
  }
}

/*
 * Set time strings to the locale specified
 */
static int
setlocaletime(const char *locale __attribute__((unused)), int selector,
              __dpmi_regs *regs __attribute__((unused)))
{
  switch (_farpeekw(selector, 7)) {
  case 0:
  default:
    strcpy(__dj_date_format, "%m/%d/%y");
    break;
  case 1:
    strcpy(__dj_date_format, "%d/%m/%y");
    break;
  case 2:
    strcpy(__dj_date_format, "%y/%m/%d");
    break;
  }
  __dj_date_format[2] = __dj_date_format[5] = _farpeekb(selector, 18);
  if (_farpeekb(selector, 24) & 1)
    strcpy(__dj_time_format, "%H:%M:%S");
  else
    strcpy(__dj_time_format, "%I:%M:%S %p");
  __dj_time_format[2] = __dj_time_format[5] = _farpeekb(selector, 20);
  return 1;
}

static const struct _cat
{
  int type;
  const char *env;
  int (*reset)(void);
  int (*set)(const char *locale, int selector, __dpmi_regs *regs);
}
cat[LC_CATEGORIES] =
{
  { LC_COLLATE, "LC_COLLATE", resetlocalecollate, setlocalecollate },
  { LC_CTYPE, "LC_CTYPE", resetlocalectype, setlocalectype },
  { LC_MONETARY, "LC_MONETARY", resetlocalemonetary, setlocalemonetary },
  { LC_NUMERIC, "LC_NUMERIC", resetlocalenumeric, setlocalenumeric },
  { LC_TIME, "LC_TIME", resetlocaletime, setlocaletime }
};

static const struct _loc2id
{
  int id;
  const char *loc;
  size_t len;
}
loc2id[] =
{
  { 43,    "de_AT",       5 },
  { 43,    "de_AT_EURO",  10 },
  { 41,    "de_CH",       5 },
  { 49,    "de_DE",       5 },
  { 49,    "de_DE_EURO",  10 },
  { 352,   "de_LU",       5 },
  { 352,   "de_LU_EURO",  10 },
  { 61,    "en_AU",       5 },
  { 32,    "en_BE",       5 },
  { 4,     "en_CA",       5 },
  { 44,    "en_GB",       5 },
  { 61,    "en_IE",       5 },
  { 61,    "en_IE_EURO",  10 },
  { 64,    "en_NZ",       5 },
  { 1,     "en_US",       5 },
  { 27,    "en_ZA",       5 },
  { 54,    "es_AR",       5 },
  { 591,   "es_BO",       5 },
  { 56,    "es_CL",       5 },
  { 57,    "es_CO",       5 },
  { 506,   "es_CR",       5 },
  { 809,   "es_DO",       5 },
  { 593,   "es_EC",       5 },
  { 34,    "es_ES",       5 },
  { 34,    "es_ES_EURO",  10 },
  { 502,   "es_GT",       5 },
  { 504,   "es_HN",       5 },
  { 52,    "es_MX",       5 },
  { 507,   "es_PA",       5 },
  { 51,    "es_PE",       5 },
  { 595,   "es_PY",       5 },
  { 503,   "es_SV",       5 },
  { 598,   "es_UY",       5 },
  { 58,    "es_VE",       5 },
  { 372,   "et_ET",       5 },
  { 358,   "fi_FI",       5 },
  { 358,   "fi_FI_EURO",  10 },
  { 32,    "fr_BE",       5 },
  { 32,    "fr_BE_EURO",  10 },
  { 2,     "fr_CA",       5 },
  { 41,    "fr_CH",       5 },
  { 352,   "fr_LU",       5 },
  { 352,   "fr_LU_EURO",  10 },
  { 33,    "fr_FR",       5 },
  { 33,    "fr_FR_EURO",  10 },
  { 30,    "gr_GR",       5 },
  { 30,    "gr_GR_EURO",  10 },
  { 972,   "he_IL",       5 },
  { 385,   "hr_HR",       5 },
  { 36,    "hu_HU",       5 },
  { 354,   "is_IS",       5 },
  { 41,    "it_CH",       5 },
  { 39,    "it_IT",       5 },
  { 39,    "it_IT_EURO",  10 },
  { 370,   "lt_LT",       5 },
  { 371,   "lv_LV",       5 },
  { 389,   "mk_MK",       5 },
  { 91,    "mr_IN",       5 },
  { 356,   "mt_MT",       5 },
  { 32,    "nl_BE",       5 },
  { 32,    "nl_BE_EURO",  10 },
  { 31,    "nl_NL",       5 },
  { 31,    "nl_NL_EURO",  10 },
  { 47,    "no_NO",       5 },
  { 48,    "pl_PL",       5 },
  { 55,    "pt_BR",       5 },
  { 351,   "pt_PT",       5 },
  { 351,   "pt_PT_EURO",  10 },
  { 40,    "ro_RO",       5 },
  { 7,     "ru_RU",       5 },
  { 38,    "sh_YU",       5 },
  { 42,    "sk_SK",       5 },
  { 386,   "sl_SI",       5 },
  { 355,   "sq_AL",       5 },
  { 381,   "sr_YU",       5 },
  { 358,   "sv_FI",       5 },
  { 46,    "sv_SE",       5 },
  { 91,    "ta_IN",       5 },
  { 200,   "th_TH",       5 },
  { 90,    "tr_TR",       5 },
  { 804,   "uk_UA",       5 },
  { 84,    "vi_VN",       5 }
};

/*
 * Set or read international environment
 */
char *
setlocale(int category, const char *locale)
{
  int honored = 1;
  int i;

  if (locale != NULL)
  {
    int segment = -1, selector = -1;
    __dpmi_regs regs = {0};
    char buf[LC_MAXNAMESIZE];
    char *p1, *p2;

    strncpy(lc_buffer, locale, LC_BUFSIZ);
    lc_buffer[LC_BUFSIZ - 1] = '\0';
    p1 = lc_buffer - 1;
    p2 = lc_buffer;
    for (i = 0; i < LC_CATEGORIES; i++)
    {
      locale = p2;
      p1 = strchr (p2, ',');
      if (p1 == NULL)
      {
        p1 = p2;
      }
      else
      {
        *p1 = '\0';
        p2 = p1 + 1;
      }
      if ((category == LC_ALL) || (cat[i].type == category))
      {
        if (*locale == '\0')
        {
          const char *env;
          if ((env = getenv (cat[i].env)) != NULL)
          {
            locale = env;
          }
          else
          if ((env = getenv ("LC_ALL")) != NULL)
          {
            locale = env;
          }
          else
          if ((env = getenv ("LANG")) != NULL)
          {
            locale = env;
          }
        }
        if (!stricmp(locale, "C") || !stricmp(locale, "POSIX"))
        {
          if (cat[i].reset() == 0)
          {
            honored = 0;
            continue;
          }
        }
        else
        {
          int CID, CCP;
          size_t j;

          /* Allocate DOS memory */
          if (segment == -1)
          {
            if ((segment = __dpmi_allocate_dos_memory(3, &selector)) == -1)
            {
              honored = 0;
              continue;
            }
          }

          /* Now try to find out the country/codepage */
          CID = 0xffff;
          CCP = 0xffff;
          if (*locale != '\0')
          {
            size_t len;
            const char *p = strchr(locale, '.');
            if (p == NULL)
              p = locale + strlen(locale);
            len = p - locale;
            for (j = 0; j < sizeof(loc2id) / sizeof(*loc2id); j++)
              if (!strncmp(locale, loc2id[j].loc,
                           len >= loc2id[j].len ? len : loc2id[j].len))
              {
                CID = loc2id[j].id;
                break;
              }
            if (*p == '.')
              CCP = atoi(p + 1);
            /* User requested the country/codepage we doesn't know about */
            if ((CID == 0xffff) || ((*p == '.') && (CCP == 0xffff)))
            {
              honored = 0;
              continue;
            }
          }

          regs.h.ah = 0x65;
          regs.h.al = 0x01;
          regs.x.bx = CCP;
          regs.x.dx = CID;
          regs.x.cx = 41;
          regs.x.es = segment;
          regs.x.di = 0;
          __dpmi_int(0x21, &regs);
          if ((regs.x.flags & 1) || (regs.x.cx != 41))
          {
            honored = 0;
            continue;
          }

          if (*locale == '\0')
          {
            CID = _farpeekw(selector, 3);
            CCP = _farpeekw(selector, 5);
            locale = "??_??";
            for (j = 0; j < sizeof(loc2id) / sizeof(*loc2id); j++)
              if (loc2id[j].id == CID)
              {
                locale = loc2id[j].loc;
                break;
              }
            sprintf(buf, "%s.%u", locale, CCP);
            locale = buf;
          }

          /* regs.x.bx, regs.x.dx, regs.x.es/di are preserved by DOS */
          if (cat[i].set(locale, selector, &regs) == 0)
          {
            honored = 0;
            continue;
          }
        }

        strlcpy(lc_current[i], locale, LC_MAXNAMESIZE);
      }
    }
    if (segment != -1)
      __dpmi_free_dos_memory(selector);
  }

  if (honored)
  {
    if (category != LC_ALL)
    {
      for (i = 0; i < LC_CATEGORIES; i++)
        if (cat[i].type == category)
          return lc_current[i];
      return NULL;
    }
    if (!stricmp(lc_current[0], lc_current[1]) &&
        !stricmp(lc_current[1], lc_current[2]) &&
        !stricmp(lc_current[2], lc_current[3]) &&
        !stricmp(lc_current[3], lc_current[4]))
    {
      return lc_current[0];
    }
    else
    {
      char *p;

      p = lc_buffer;
      for (i = 0; i < LC_CATEGORIES; i++)
      {
	size_t len = strlen(lc_current[i]);
        memcpy(p, lc_current[i], len + 1);
        p += len;
        if (i != (LC_CATEGORIES - 1))
          *p++ = ',';
      }
      return lc_buffer;
    }
  }
  else
    return NULL;
}

#ifdef TEST

extern unsigned char __dj_collate_table[256];
extern char __dj_date_format[];
extern char __dj_time_format[];

int
main(int ac, char *av[])
{
  int i;
  const char *loc = (ac == 1) ? "" : av[1];
  char *lc;
  lc = setlocale(LC_ALL, loc);
  lc = setlocale(LC_ALL, NULL);
  printf("Locale: %s\n", lc ? lc : "not detected");
  for (i = 32; i < 256; i++)
    printf("%c%c%c|", (char) i, tolower(i), toupper(i));
  printf("\n");
  for (i = 0; i < 256; i++)
    printf("%02xh ", __dj_collate_table[i]);
  printf("\n%f\n%s %s\n", 1000456.23, __dj_date_format, __dj_time_format);
  return 0;
}
#endif
