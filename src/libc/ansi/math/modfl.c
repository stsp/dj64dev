//#include "libm.h"
#include <math.h>
#include <stdint.h>

long double modfl(long double x, long double *iptr)
{
	double d;
	long double r;

	r = modf(x, &d);
	*iptr = d;
	return r;
}
