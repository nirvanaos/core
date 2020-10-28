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
	static const unsigned MAX_COUNT = 10; // 2 ^ 10 iterations
	static const unsigned SLEEP_ITERATIONS = 200;
public:
	BackOff () :
		count_ (0)
	{}

	~BackOff ()
	{}

	void operator () ();

private:
	inline void cpu_relax ();

private:
	unsigned count_;
	RandomGen rndgen_;
};

}
}

#endif
