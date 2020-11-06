#include "BackOff.h"
#include <random>
#if defined (_MSVC_LANG)
#include <intrin.h>
#endif

namespace Nirvana {
namespace Core {

inline void BackOff::cpu_relax ()
{
#if defined (_MSVC_LANG)
	_mm_pause ();
#elif defined (__GNUG__) || defined (__clang__)
	asm ("pause");
#endif
}

void BackOff::operator () ()
{
	if ((iterations_ <<= 1) > ITERATIONS_MAX)
		iterations_ = ITERATIONS_MAX;

	// TODO: We can try other distributions: geometric, exponential...
	typedef std::uniform_int_distribution <UWord> Dist;

	UWord iters = Dist (1U, iterations_)(rndgen_);
	if (ITERATIONS_MAX <= ITERATIONS_YIELD || iters < ITERATIONS_YIELD) {
		volatile UWord cnt = iters;
		do
			cpu_relax ();
		while (--cnt);
	} else
		Port::BackOff::yield (iters);
}

}
}
