/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <libc/stubs.h>
#include <time.h>
#include <dpmi.h>

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
  __dpmi_regs r = {0};
  struct tm tmblk;
  struct timeval tv_tmp;


  if (!tv)
    tv = &tv_tmp;

  r.h.ah = 0x2c;
  __dpmi_int(0x21, &r);

  tv->tv_usec = r.h.dl * 10000;
  tmblk.tm_sec = r.h.dh;
  tmblk.tm_min = r.h.cl;
  tmblk.tm_hour = r.h.ch;

  r.h.ah = 0x2a;
  __dpmi_int(0x21, &r);

  tmblk.tm_mday = r.h.dl;
  tmblk.tm_mon = r.h.dh - 1;
  tmblk.tm_year = (r.x.cx & 0x7ff) - 1900;

  tmblk.tm_wday = tmblk.tm_yday = tmblk.tm_gmtoff = 0;
  tmblk.tm_zone = 0;
  tmblk.tm_isdst = -1;

  tv->tv_sec = mktime(&tmblk);

  if(tz)
  {
    struct tm *tmloc = localtime(&(tv->tv_sec));
    tz->tz_minuteswest = - tmloc->tm_gmtoff / 60;
    tz->tz_dsttime = tmloc->tm_isdst;
  }
  return 0;
}
