// Nirvana core implementation definitions

#ifndef NIRVANA_CORE_H_
#define NIRVANA_CORE_H_

#include <ORB.h>
#include <cpplib.h>
#include <assert.h>
#include "config.h"

#undef verify

#ifdef NDEBUG

#define verify(exp) (exp)

#else

#define verify(exp) assert(exp)

#endif

// Endian order
#define ENDIAN_ORDER  ('ABCD')
#define LITTLE_ENDIAN 0x41424344UL 
#define BIG_ENDIAN    0x44434241UL
#define PDP_ENDIAN    0x42414443UL

namespace Nirvana {

// Number of leading zeros
// http://www.hackersdelight.org/

#if (defined(_M_IX86) || defined(_M_AMD64))
#define NLZ_IMPL_DOUBLE
#endif

inline uint32_t nlz (uint32_t x)
{
#ifdef NLZ_IMPL_DOUBLE
	union
	{
		uint32_t as_int [2];
		double as_double;
	};

	as_double = (double)x + 0.5;

	static const size_t LE =
#if ENDIAN_ORDER == LITTLE_ENDIAN
		1;
#elif ENDIAN_ORDER == BIG_ENDIAN
		0;
#else
#error Unknown byte order.
#endif
		return 1054 - (as_int [LE] >> 20);
#else
	int32_t y, m, n;

	y = -(int32_t)(x >> 16);      // If left half of x is 0,
	m = (y >> 16) & 16;  // set n = 16.  If left half
	n = 16 - m;          // is nonzero, set n = 0 and
	x = x >> m;          // shift x right 16.
											 // Now x is of the form 0000xxxx.
	y = x - 0x100;       // If positions 8-15 are 0,
	m = (y >> 16) & 8;   // add 8 to n and shift x left 8.
	n = n + m;
	x = x << m;

	y = x - 0x1000;      // If positions 12-15 are 0,
	m = (y >> 16) & 4;   // add 4 to n and shift x left 4.
	n = n + m;
	x = x << m;

	y = x - 0x4000;      // If positions 14-15 are 0,
	m = (y >> 16) & 2;   // add 2 to n and shift x left 2.
	n = n + m;
	x = x << m;

	y = x >> 14;         // Set y = 0, 1, 2, or 3.
	m = y & ~(y >> 1);   // Set m = 0, 1, 2, or 2 resp.
	return n + 2 - m;
#endif
}

}

#endif
