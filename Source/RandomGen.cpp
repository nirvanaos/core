// Nirvana project.
// Random number generator

#include "RandomGen.h"

namespace Nirvana {
namespace Core {

RandomGen::RandomGen () :
	std::mt19937 ((unsigned)(intptr_t)this)
{}

}
}
