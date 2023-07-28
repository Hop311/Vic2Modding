#pragma once

#include "types.h"

#define MIN(x, y) ((x) <= (y) ? (x) : (y))
#define MAX(x, y) ((x) >= (y) ? (x) : (y))

//#define CLAMP(x,a,b) ((x) <= (a) ? (a) : ((x) >= (b) ? (b) : (x)))
//#define ABS(x) ((x) < 0 ? -(x) : (x))
//#define NABS(x) ((x) > 0 ? -(x) : (x))
#define SWAP(x,y) do \
   { u8 swap_temp[sizeof(x) == sizeof(y) ? (signed)sizeof(x) : -1]; \
	   memcpy(swap_temp, &y, sizeof(x)); \
	   memcpy(&y, &x, sizeof(x)); \
	   memcpy(&x, swap_temp, sizeof(x)); \
   } while(0)

float truncate_int_part(float x);
