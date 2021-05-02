#include "Process.h"

namespace Nirvana {
namespace Legacy {
namespace Core {

Nirvana::Core::Heap& Process::memory () NIRVANA_NOEXCEPT
{
	return heap_;
}

}
}
}
