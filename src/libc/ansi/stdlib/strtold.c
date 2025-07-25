/* Copyright (C) 2015 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2013 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2008 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2002 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1999 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <libc/stubs.h>
#include <locale.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <libc/unconst.h>
#include <libc/ieee.h>


#define MANTISSA_SIZE       (64)     /*  Number binary digits in the integer and fractional part of the mantissa.  */
#define HEX_DIGIT_SIZE      (4)
#define LONG_DOUBLE_BIAS    (0x3FFFU)
#define MAX_BIN_EXPONENT    (16383)   /*  Max. and min. binary exponent (inclusive) as  */
#define MIN_BIN_EXPONENT    (-16382)  /*  defined in Intel manual (253665.pdf, Table 4.2).  */
#define IS_ZERO_DIGIT(x)    ((x) == '0')
#define IS_DEC_DIGIT(x)     (((x) >= '0') && ((x) <= '9'))
#define IS_HEX_DIGIT(x)     ((((x) >= 'A') && ((x) <= 'F')) || \
                             (((x) >= 'a') && ((x) <= 'f')) || \
                             IS_DEC_DIGIT(x))
#define IS_DEC_EXPONENT(x)  (((x[0]) == 'E' || (x[0]) == 'e') && \
                             ((x[1] == '+' &&  IS_DEC_DIGIT(x[2])) || \
                              (x[1] == '-' &&  IS_DEC_DIGIT(x[2])) || \
                             IS_DEC_DIGIT(x[1])))
#define IS_HEX_EXPONENT(x)  (((x[0]) == 'P' || (x[0]) == 'p') && \
                             ((x[1] == '+' &&  IS_DEC_DIGIT(x[2])) || \
                              (x[1] == '-' &&  IS_DEC_DIGIT(x[2])) || \
                             IS_DEC_DIGIT(x[1])))

static long double powten[] =
{
  1e1L, 1e2L, 1e4L, 1e8L, 1e16L, 1e32L, 1e64L, 1e128L, 1e256L,
//  1e512L, 1e1024L, 1e2048L, 1e4096L
};

#define _countof(array) (sizeof(array) / sizeof(array[0]))

long double
strtold(const char *s, char **sret)
{
  long double r = 0.0L;         /* result */
  int e = 0, ne = 0;            /* exponent */
  int sign = 1;                 /* +- 1.0 */
  int esign = 1;
  int flags = 0;
  int l2powm1;
  char radix_point = localeconv()->decimal_point[0];


  if (sret)
    *sret = unconst(s, char *);

  while(isspace((unsigned char)*s))
    s++;

  if (!*s)
    return 0.0;

  /* Handle leading sign. */
  if (*s == '+')
    s++;
  else if (*s == '-')
  {
    sign = -1;
    s++;
  }

  /* Handle INF and INFINITY. */
  if (!strnicmp("INF", s, 3))
  {
    if (sret)
    {
      if (!strnicmp("INITY", &s[3], 5))
        *sret = unconst((&s[8]), char *);
      else
        *sret = unconst((&s[3]), char *);
    }

    return (sign < 0) ? -INFINITY : INFINITY;
  }

  /* Handle NAN and NAN(<whatever>). */
  if (!strnicmp("NAN", s, 3))
  {
    _longdouble_union_t t;

    t.ld = NAN;


    if (sign < 0)
      t.ldt.sign = 1;

    if (s[3] == '(')
    {
      unsigned long long mantissa_bits = 0;
      char *endptr = unconst((&s[4]), char *);

      mantissa_bits = strtoull(&s[4], &endptr, 0);
      if (*endptr == ')')
      {
        mantissa_bits = mantissa_bits & 0x7fffffffffffffffULL;  /* Ignore integer bit. */
        if (mantissa_bits)
        {
          t.ldt.mantissal = mantissa_bits & 0xffffffff;
          t.ldt.mantissah = ((mantissa_bits >> 32) & 0xffffffff ) | 0x80000000;
        }
        if (sret)
          *sret = endptr + 1;

        return t.ld;
      }

      /* The subject sequence didn't match NAN(<number>),
         so match only NAN. */
    }

    if (sret)
      *sret = unconst((&s[3]), char *);

    return t.ld;
  }

  /* Handle 0xH.HHH[p|P][+|-]DDD. */
  if (!strnicmp("0x", s, 2) && (s[2] == '.' || IS_HEX_DIGIT(s[2])))
  {
    const char *next_char = NULL;
    const int max_digits = MANTISSA_SIZE / HEX_DIGIT_SIZE;  /* The exact number of digits that fits in mantissa.  */
    int bin_exponent, digits, integer_digits;
    unsigned long long int mantissa, msb_mask;
    _longdouble_union_t ieee754;


    /*
     *  Mantissa.
     *  16 hex digits fit into the mantissa
     *  including the explicit integer bit.
     */
    bin_exponent = 0;
    integer_digits = 0;
    msb_mask = mantissa = 0x00ULL;
    s += 2;  /*  Skip the hex prefix.  */
    while (integer_digits < max_digits && IS_HEX_DIGIT(*s))
    {
      flags = 1;
      mantissa <<= HEX_DIGIT_SIZE;
      mantissa |= IS_DEC_DIGIT(*s) ? *s - '0' :
                  ((*s >= 'A') && (*s <= 'F')) ? *s - 'A' + 10 : *s - 'a' + 10;
      if (mantissa)        /*  Discarts leading zeros.  */
        integer_digits++;  /*  Counts hex digits.  16**integer_digits.  */
      s++;
    }
    if (integer_digits)
    {
      /*
       *  Compute the binary exponent for a normalized mantissa by
       *  shifting the radix point inside the most significant hex digit.
       */

      next_char = s;
      for (digits = 0; IS_HEX_DIGIT(*s); s++)
        digits++;  /*  Counts hex digits.  */

      msb_mask = 0x01ULL;
      bin_exponent = integer_digits * HEX_DIGIT_SIZE - 1;  /*  2**bin_exponent.  */
      for (msb_mask <<= bin_exponent; !(mantissa & msb_mask); msb_mask >>= 1)
        bin_exponent--;
      bin_exponent += digits * HEX_DIGIT_SIZE;
      integer_digits += digits;
    }

    digits = integer_digits;
    if (*s == radix_point)
    {
      int extra_shifts, fraction_zeros = 0;

      s++;
      digits = integer_digits;
      while ((digits - fraction_zeros) < max_digits && IS_HEX_DIGIT(*s))
      {
        flags = 1;
        digits++;  /*  Counts hex digits.  */
        mantissa <<= HEX_DIGIT_SIZE;
        mantissa |= IS_DEC_DIGIT(*s) ? *s - '0' :
                    ((*s >= 'A') && (*s <= 'F')) ? *s - 'A' + 10 : *s - 'a' + 10;
        if (mantissa == 0)
          fraction_zeros++;  /*  Counts hex zeros.  16**(-fraction_zeros + 1).  */
        s++;
      }
      next_char = s;
      if (!integer_digits && mantissa)
      {
        /*
         *  Compute the binary exponent for a normalized mantissa by
         *  shifting the radix point inside the most significant hex digit.
         */

        msb_mask = 0x01ULL;
        bin_exponent = -fraction_zeros * HEX_DIGIT_SIZE;  /*  2**bin_exponent.  */
        for (msb_mask <<= (digits * HEX_DIGIT_SIZE + bin_exponent); !(mantissa & msb_mask); msb_mask >>= 1)
          bin_exponent--;
      }
      else if ((extra_shifts = digits - integer_digits) > 0)
        msb_mask <<= extra_shifts * HEX_DIGIT_SIZE;
    }

    if (flags == 0)
    {
      errno = EINVAL;  /*  No valid mantissa, no conversion could be performed.  */
      return 0.0L;
    }

    if (sret)
      *sret = unconst(s, char *);

    if (digits >= max_digits)
    {
      /*
       *  Round half towards plus infinity (round half up).
       */
      const int lsd = IS_DEC_DIGIT(*next_char) ? *next_char - '0' :  /*  Least significant hex digit.  Will be rounded out.  */
                      ((*next_char >= 'A') && (*next_char <= 'F')) ? *next_char - 'A' + 10 : *next_char - 'a' + 10;
      if (lsd > 0x07)
      {
        mantissa += 0x0000000000000001ULL;  /* Smallest float greater than x.  */
        if (!(mantissa & msb_mask))
        {
          /*  Overflow.  */
          mantissa >>= 1;
          mantissa |= 0x8000000000000000ULL;
          bin_exponent++;
        }
      }
    }

    if (mantissa)
    {
      /*
       *  Normalize mantissa.
       */
      for (digits = 0; !(mantissa & 0x8000000000000000ULL); digits++)
        mantissa <<= 1;  /*  Shift a binary 1 into the integer part of the mantissa.  */
      /*  At this point the mantissa is normalized and the exponent has been adjusted accordingly.  */
    }


    /*
     *  After discarting all hex digits left,
     *  if the next character is P or p
     *  continue with the extracting of the
     *  exponent, else any other character
     *  that have appeared terminates the number.
     */
    while (IS_HEX_DIGIT(*s))
      s++;

    /*
     *  Exponent.
     */
    if (IS_HEX_EXPONENT(s))
    {
      long long int exponent = 0.0L;

      s++;
      if (*s == '+')
        s++;
      else if (*s == '-')
      {
        esign = -1;
        s++;
      }

      while ((esign * exponent + bin_exponent) < (MAX_BIN_EXPONENT + 1) && IS_DEC_DIGIT(*s))
      {
        exponent *= 10;
        exponent += *s - '0';
        s++;
      }
      bin_exponent += esign * exponent;  /*  2**bin_exponent.  */
      while (IS_DEC_DIGIT(*s))
        s++;  /*  Discart rest of exponent.  */
    }


    if (sret)
      *sret = unconst(s, char *);

    if (mantissa)
    {
      if (bin_exponent > MAX_BIN_EXPONENT)
      {
        errno = ERANGE;
        return sign * HUGE_VALL;
      }
      else if(bin_exponent < MIN_BIN_EXPONENT)
      {
        errno = ERANGE;
        return 0.0L;
      }
      ieee754.ldt.sign      = (sign == 1) ? 0 : 1;
      ieee754.ldt.exponent  = 0x7FFFU & (bin_exponent + LONG_DOUBLE_BIAS);
      ieee754.ldt.mantissah = 0xFFFFFFFFUL & (mantissa >> 32);
      ieee754.ldt.mantissal = 0xFFFFFFFFUL & mantissa;
    }
    else
      ieee754.ld = sign * 0.0L;

    return ieee754.ld;
  }

  /* Handle ordinary numbers. */
  while (IS_DEC_DIGIT(*s))
  {
    flags |= 1;
    r *= 10.0L;
    r += *s - '0';
    s++;
  }

  if (*s == radix_point)
  {
    s++;
    while (IS_DEC_DIGIT(*s))
    {
      flags |= 2;
      r *= 10.0L;
      r += *s - '0';
      s++;
      ne++;
    }
  }
  if (flags == 0)
  {
    errno = EINVAL;  /*  No valid mantissa, no conversion could be performed.  */
    return 0.0L;
  }

  if (sret)
    *sret = unconst(s, char *);

  if (IS_DEC_EXPONENT(s))
  {
    s++;
    if (*s == '+')
      s++;
    else if (*s == '-')
    {
      s++;
      esign = -1;
    }
    while (IS_DEC_DIGIT(*s))
    {
      e *= 10;
      e += *s - '0';
      s++;
    }
  }
  if (esign < 0)
  {
    esign = -esign;
    e = -e;
  }
  e = e - ne;
#if 0
  if (e < -4096)
  {
    /* possibly subnormal number, 10^e would overflow */
    r *= 1.0e-2048L;
    e += 2048;
  }
#endif
  if (e < 0)
  {
    e = -e;
    esign = -esign;
  }
  if (e >= 8192)
    e = 8191;
  if (e)
  {
    long double d = 1.0L;
    l2powm1 = 0;
    while (e && l2powm1 < (int)_countof(powten))
    {
      if (e & 1)
        d *= powten[l2powm1];
      e >>= 1;
      l2powm1++;
    }
    if (esign > 0)
      r *= d;
    else
      r /= d;
  }

  if (sret)
    *sret = unconst(s, char *);

  return r * sign;
}

#ifdef __APPLE__
long double _strtold(const char *s, char **sret)
{
  return strtold(s, sret);
}
#else
long double __attribute__((alias("strtold")))
_strtold(const char *s, char **sret);
#endif
