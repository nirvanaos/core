// Nirvana project.
// Atomic pseudorandom number generator.
// We use fast and non-cryptographically secure Xorshift algorithm.
// http://www.jstatsoft.org/v08/i14/paper

#ifndef NIRVANA_CORE_RANDOMGEN_H_
#define NIRVANA_CORE_RANDOMGEN_H_

#include <Nirvana/Nirvana.h>
#include <limits>

namespace Nirvana {
namespace Core {

/// Random-number generator.
class RandomGen
{
public:
	typedef UWord result_type;

	RandomGen () : // Use `this` as seed value
		state_ ((result_type)reinterpret_cast <uintptr_t> (this))
	{}

	RandomGen (result_type seed) :
		state_ (seed)
	{}

	static result_type min ()
	{
		return 0;
	}

	static result_type max ()
	{
		return std::numeric_limits <result_type>::max ();
	}

	result_type operator () ();

protected:
	static uint16_t xorshift (uint16_t x);
	static uint32_t xorshift (uint32_t x);
	static uint64_t xorshift (uint64_t x);

protected:
	result_type state_;
};

class RandomGenAtomic :
	public RandomGen
{
public:
	result_type operator () ();
};

}
}

#endif

