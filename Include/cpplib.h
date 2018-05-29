// Nirvana project
// CPP general purpose utilities

#ifndef NIRVANA_CPPLIB_H_
#define NIRVANA_CPPLIB_H_

#include <BasicTypes.h>
#include <limits.h>

namespace Nirvana {

// Useful templates to zero memory

template <class It>
inline void zero (It begin, It end)
{
	while (begin != end)
		*(begin++) = 0;
}

template <class C>
inline void align_zero (C* begin, C* end)
{
	C* aligned_begin = reinterpret_cast <C*> (((UWord)begin + sizeof (UWord) - 1) & ~(sizeof (UWord) - 1));
	C* aligned_end = reinterpret_cast <C*> (((UWord)end + sizeof (UWord) - 1) & ~(sizeof (UWord) - 1));
	if (aligned_begin < aligned_end) {

		while (begin != aligned_begin)
			*(begin++) = 0;

		do {
			*reinterpret_cast <UWord*> (begin) = 0;
			begin += sizeof (UWord) / sizeof (C);
		} while (begin != aligned_end);
	}

	while (begin != end)
		*(begin++) = 0;
}

template <>
inline void zero (signed char* begin, signed char* end)
{
	align_zero (begin, end);
}

template <>
inline void zero (unsigned char* begin, unsigned char* end)
{
	align_zero (begin, end);
}

#if (SHRT_MAX < INT_MAX)

template <>
inline void zero (signed short* begin, signed short* end)
{
	align_zero (begin, end);
}

template <>
inline void zero (unsigned short* begin, unsigned short* end)
{
	align_zero (begin, end);
}

}

#endif

#endif
