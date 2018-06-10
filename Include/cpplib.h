// Nirvana project
// CPP general purpose utilities

#ifndef NIRVANA_CPPLIB_H_
#define NIRVANA_CPPLIB_H_

#include <BasicTypes.h>
#include <limits.h>

namespace Nirvana {

// Integral rounding

template <typename I>
inline I round_down (I i, size_t n2)
{
	return (I)((size_t)i / n2 * n2);
}

template <typename I>
inline I round_up (I i, size_t n2)
{
	return (I)(((size_t)i + n2 - 1) / n2 * n2);
}

// Zero memory

template <class It>
inline void zero (It begin, It end)
{
	while (begin != end)
		*(begin++) = 0;
}

}

#endif
