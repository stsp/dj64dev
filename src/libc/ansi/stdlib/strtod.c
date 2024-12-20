/* Copyright (C) 2015 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2013 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1997 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <libc/stubs.h>
#include <locale.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <libc/unconst.h>
#include <libc/ieee.h>


#define MANTISSA_SIZE       (52)     /*  Number binary digits in the fractional part of the mantissa.  */
#define HEX_DIGIT_SIZE      (4)
#define DOUBLE_BIAS         (0x3FFU)
#define MAX_BIN_EXPONENT    (1023)   /*  Max. and min. binary exponent (inclusive) as  */
#define MIN_BIN_EXPONENT    (-1022)  /*  defined in Intel manual (253665.pdf, Table 4.2).  */
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


double
strtod(const char *s, char **sret)
{
  long double r = 0;            /* result */
  int e = 0;                    /* exponent */
  long double d;                /* scale */
  int sign = 1;                 /* +- 1.0 */
  int esign = 1;
  int i;
  int flags = 0;
  char radix_point = localeconv()->decimal_point[0];


  if (sret)
    *sret = unconst(s, char *);

  while (isspace((unsigned char) *s))
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
    _double_union_t t;

    t.d = NAN;

    if (sign < 0)
      t.dt.sign = 1;

    if (s[3] == '(')
    {
      unsigned long long mantissa_bits = 0;
      char *endptr = unconst((&s[4]), char *);

      mantissa_bits = strtoull(&s[4], &endptr, 16);
      if (*endptr == ')')
      {
        mantissa_bits = mantissa_bits & 0xfffffffffffffULL;
        if (mantissa_bits)
        {
          t.dt.mantissal = mantissa_bits & 0xffffffff;
          t.dt.mantissah = (mantissa_bits >> 32) & 0xfffff;
        }
        if (sret)
          *sret = endptr + 1;

        return t.d;
      }

      /* The subject sequence didn't match NAN(<hex-number>),
         so match only NAN. */
    }

    if (sret)
      *sret = unconst((&s[3]), char *);

    return t.d;
  }

  /* Handle 0xH.HHH[p|P][+|-]DDD. */
  if (!strnicmp("0x", s, 2) && (s[2] == '.' || IS_HEX_DIGIT(s[2])))
  {
    const int max_digits = 1 + MANTISSA_SIZE / HEX_DIGIT_SIZE + 1;  /* Two more digits than fit into mantissa.  */
    int bin_exponent, digits, integer_digits;
    unsigned long long int mantissa, msb_mask;
    _double_union_t ieee754;


    /*
     *  Mantissa.
     *  13 hex digits fit into the fraction part.
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
      const int lsd = 0x000000000000000FULL & mantissa;  /*  Least significant hex digit.  Will be rounded out.  */
      if (lsd > 0x07)
      {
        mantissa += 0x0000000000000010ULL;  /* Smallest float greater than x.  */
        if (!(mantissa & msb_mask))
        {
          /*  Overflow.  */
          mantissa >>= 1;
          bin_exponent++;
        }
      }
    }

    if (mantissa)
    {
      /*
       *  Normalize mantissa.
       */
      while (!(mantissa & 0x8000000000000000ULL))
        mantissa <<= 1;  /*  Shift a binary 1 into the integer part of the mantissa.  */
      mantissa >>= (63 - 52);
      /*  At this point the mantissa is normalized and the exponent has been adjusted accordingly.  */
    }


    /*
     *  After discarting all hex digits left,
     *  if the next character is P or p
     *  continue with the extraction of the
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
      long int exponent = 0.0;

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
        return sign * HUGE_VAL;
      }
      else if(bin_exponent < MIN_BIN_EXPONENT)
      {
        errno = ERANGE;
        return 0.0;
      }
      ieee754.dt.sign      = (sign == 1) ? 0 : 1;
      ieee754.dt.exponent  = 0x7FFFU & (bin_exponent + DOUBLE_BIAS);
      ieee754.dt.mantissah = 0x000FFFFFUL & (mantissa >> 32);
      ieee754.dt.mantissal = 0xFFFFFFFFUL & mantissa;
    }
    else
      ieee754.d = sign * 0.0;

    return ieee754.d;
  }

  /* Handle ordinary numbers. */
  while (IS_DEC_DIGIT(*s))
  {
    flags |= 1;
    r *= 10.0;
    r += *s - '0';
    s++;
  }

  if (*s == radix_point)
  {
    d = 0.1L;
    s++;
    while (IS_DEC_DIGIT(*s))
    {
      flags |= 2;
      r += d * (*s - '0');
      s++;
      d *= 0.1L;
    }
  }
  if (flags == 0)
  {
    errno = EINVAL;  /*  No valid mantissa, no conversion could be performed.  */
    return 0.0;
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

  /* Detect overflow.  */
  if (e < 0)
  {
    errno = ERANGE;
    r = HUGE_VAL;
  }
  else if (esign < 0)
  {
    const int exponent = e + 1;
    for (i = 1; i < exponent; i++)
    {
      r *= 0.1L;
      /* Detect underflow below 2^-1075, which is half
         the smallest representable double. */
#if 0
      if (r < 2.47032822920623272088e-324L)
#else
      if (r < 5e-324L)
#endif
      {
        errno = ERANGE;
        r = 0.0;
        break;
      }
    }
  }
  else
  {
    const int exponent = e + 1;
    for (i = 1; i < exponent; i++)
    {
      r *= 10.0;
      if (r > DBL_MAX)  /* detect overflow */
      {
        errno = ERANGE;
        r = HUGE_VAL;
        break;
      }
    }
  }

  if (sret)
    *sret = unconst(s, char *);

  return r * sign;
}
