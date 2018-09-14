// Nirvana project.
// Random number generator

#include "RandomGen.h"

namespace Nirvana {
namespace Core {

uint32_t RandomGen::operator () ()
{
	/* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
	uint32_t x = state_;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	state_ = x;
	return x;
}

}
}
