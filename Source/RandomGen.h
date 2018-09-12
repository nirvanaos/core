// Nirvana project.
// Pseudorandom number generator.
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

  RandomGen () :
    state_ (reinterpret_cast <uint32_t> (this))
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

private:
  uint32_t state_;
};

}
}

#endif

