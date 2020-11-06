#ifndef NIRVANA_CORE_BACKOFF_H_
#define NIRVANA_CORE_BACKOFF_H_

#include "core.h"
#include <Port/BackOff.h>
#include "RandomGen.h"

namespace Nirvana {
namespace Core {

class BackOff :
	private Port::BackOff
{
public:
	BackOff () :
		iterations_ (1)
	{}

	~BackOff ()
	{}

	void operator () ();

private:
	/// A number of iterations when we should call yield ().
	using Port::BackOff::ITERATIONS_MAX;

	/// Maximal number of iterations.
	using Port::BackOff::ITERATIONS_YIELD;

	/// If ITERATIONS_MAX == ITERATIONS_YIELD we never call yield ().
	static_assert (ITERATIONS_MAX >= ITERATIONS_YIELD, "ITERATIONS_MAX >= ITERATIONS_YIELD");

	inline void cpu_relax ();

private:
	UWord iterations_;
	RandomGen rndgen_;
};

}
}

#endif
