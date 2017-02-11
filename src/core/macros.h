#pragma once

#include <math.h>

//#define M_PI 3.14159265359

#define ROUND_2_INT(f) ((int)(f >= 0.0 ? (f + 0.5) : (f - 0.5)))

#define MIN(a, b) ((a > b) ? b : a)
#define MAX(a, b) ((a < b) ? b : a)

#define CEILH(c) (ceil(c) + ((c == ceil(c)) ? 1.0 : 0.0))

#define CEILN(c) ((c == 0.0) ? 0.0 : CEILH(c))