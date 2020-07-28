// Nirvana project.
// Random number generator

#include "RandomGen.h"
#include <atomic>

namespace Nirvana {
namespace Core {

using namespace std;

inline
uint32_t RandomGen::xorshift (uint32_t x)
{
	/* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return x;
}

uint32_t RandomGen::operator () ()
{
	return state_ = xorshift (state_);
}

uint32_t RandomGenAtomic::operator () ()
{
	/* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
	uint32_t s = atomic_load((const volatile atomic<uint32_t>*)&state_);
	uint32_t x;
	do {
		x = xorshift (s);
	} while (!atomic_compare_exchange_weak ((volatile atomic<uint32_t>*)&state_, &s, x));
	return x;
}

}
}
