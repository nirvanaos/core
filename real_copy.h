#ifndef NIRVANA_CORE_REAL_COPY_H_
#define NIRVANA_CORE_REAL_COPY_H_

#include <BasicTypes.h>

namespace Nirvana {

using namespace ::CORBA;

template <class InIt, class OutIt>
inline OutIt real_copy (InIt begin, InIt end, OutIt dst)
{
	while (begin != end)
		*(dst++) = *(begin++);

	return dst;
}

// Partial specialization for performance

template <>
Octet* real_copy (const Octet* begin, const Octet* end, Octet* dst);

template <class BidIt1, class BidIt2>
inline void real_move (BidIt1 begin, BidIt1 end, BidIt2 dst)
{
	if (dst <= begin || dst >= end)
		while (begin != end)
			*(dst++) = *(begin++);
	else
		while (begin != end)
			*(--dst) = *(--end);
}

// Partial specialization for performance

template <>
void real_move (const Octet* begin, const Octet* end, Octet* dst);

}

#endif
