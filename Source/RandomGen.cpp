// Nirvana project.
// Random number generator

#include "RandomGen.h"

namespace Nirvana {
namespace Core {

uint32_t RandomGen::operator () ()
{
	/* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
	uint32_t s = state_.load ();
	uint32_t x;
	do {
		x = s;
		x ^= x << 13;
		x ^= x >> 17;
		x ^= x << 5;
	} while (!state_.compare_exchange_weak (s, x));
	return x;
}

}
}
