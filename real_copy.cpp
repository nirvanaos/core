#include "real_copy.h"
#include <Nirvana.h>

namespace Nirvana {

using namespace ::CORBA;

template <>
Octet* real_copy (const Octet* begin, const Octet* end, Octet* dst)
{
	const Octet* aligned_begin = round_up (begin, sizeof (UWord));
	const Octet* aligned_end = round_down (end, sizeof (UWord));

	while (begin != aligned_begin)
		*(dst++) = *(begin++);

	if (aligned_begin < aligned_end) {

		while (begin != aligned_begin)
			*(dst++) = *(begin++);

		do {
			*(UWord*)dst = *(UWord*)begin;
			dst += sizeof (UWord);
			begin += sizeof (UWord);
		} while (begin != aligned_end);

	}

	while (begin != end)
		*(dst++) = *(begin++);

	return dst;
}

template <>
void real_move (const Octet* begin, const Octet* end, Octet* dst)
{
	const Octet* aligned_begin = round_up (begin, sizeof (UWord));
	const Octet* aligned_end = round_down (end, sizeof (UWord));

	if (dst <= begin || dst >= end) {

		if (aligned_begin != aligned_end) {

			while (begin != aligned_begin)
				*(dst++) = *(begin++);

			do {
				*(UWord*)dst = *(UWord*)begin;
				dst += sizeof (UWord);
				begin += sizeof (UWord);
			} while (begin != aligned_end);

		}

		while (begin != end)
			*(dst++) = *(begin++);

	} else {

		if (aligned_begin < aligned_end) {

			while (end != aligned_end)
				*(--dst) = *(--end);

			do {
				dst -= sizeof (UWord);
				end -= sizeof (UWord);
				*(UWord*)dst = *(UWord*)end;
			} while (end != aligned_begin);

		}

		while (end != begin)
			*(--dst) = *(--end);

	}
}

}
