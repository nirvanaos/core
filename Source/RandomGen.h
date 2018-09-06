// Nirvana project.
// Random number generator

#ifndef NIRVANA_CORE_RANDOMGEN_H_
#define NIRVANA_CORE_RANDOMGEN_H_

#include <random>

namespace Nirvana {
namespace Core {

/// Random-number generator.
class RandomGen : public std::mt19937
{
public:
	RandomGen ();
};

}
}

#endif

