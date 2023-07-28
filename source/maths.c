#include "maths.h"

#include <math.h>

/*u32 clamp(u32 min, u32 val, u32 max) {
	if (val <= min) return min;
	if (val >= max) return max;
	return val;
}
void swap(u32 *a, u32 *b) {
	*a ^= *b;
	*b ^= *a;
	*a ^= *b;
}*/

float truncate_int_part(float x) {
	x -= (float)(int)x;
	return x + (float)(x < 0.0f);
}