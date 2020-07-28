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
	RandomGen::result_type max_iters = 1 << count_;

	// TODO: Implement some algorithm to choice the back off time
	typedef std::uniform_int_distribution <RandomGen::result_type> Dist;

	unsigned iters = Dist (1, max_iters)(rndgen_);
	if (iters < SLEEP_ITERATIONS)
		for (volatile unsigned i = 0; i < iters; ++i)
			cpu_relax ();
	else
		Port::BackOff::sleep ((iters - SLEEP_ITERATIONS) / SLEEP_ITERATIONS);

	if (count_ < MAX_COUNT)
		++count_;
}

}
}
