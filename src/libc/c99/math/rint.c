#include "float.h"
#include <math.h>
#include <stdint.h>

#define EPS DBL_EPSILON
static const double toint = 1/EPS;

double rint(double x)
{
	union {double f; uint64_t i;} u = {x};
	int e = u.i>>52 & 0x7ff;
	int s = u.i>>63;
	double y;

	if (e >= 0x3ff+52)
		return x;
	if (s)
		y = x - toint + toint;
	else
		y = x + toint - toint;
	if (y == 0)
		return s ? -0.0 : 0;
	return y;
}
