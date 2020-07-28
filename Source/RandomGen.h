// Nirvana project.
// Atomic pseudorandom number generator.
// We use fast and non-cryptographically secure Xorshift algorithm.
// http://www.jstatsoft.org/v08/i14/paper

#ifndef NIRVANA_CORE_RANDOMGEN_H_
#define NIRVANA_CORE_RANDOMGEN_H_

#include <stdint.h>
#include <limits>

namespace Nirvana {
namespace Core {

/// Random-number generator.
class RandomGen
{
public:
	typedef uint32_t result_type;

	RandomGen () : // Use `this` as seed value
		state_ ((uint32_t)reinterpret_cast <uintptr_t> (this))
	{}

	RandomGen (uint32_t s) :
		state_ (s)
	{}

	static uint32_t min ()
	{
		return 0;
	}

	static uint32_t max ()
	{
		return std::numeric_limits <uint32_t>::max ();
	}

	uint32_t operator () ();

protected:
	static uint32_t xorshift (uint32_t x);

protected:
	uint32_t state_;
};

class RandomGenAtomic :
	public RandomGen
{
public:
	uint32_t operator () ();
};

}
}

#endif

