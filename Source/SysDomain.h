// Nirvana project.
// System domain.
#ifndef NIRVANA_CORE_SYSDOMAIN_H_
#define NIRVANA_CORE_SYSDOMAIN_H_

#ifdef _WIN32
#include "Windows/SysDomainBase.h"
namespace Nirvana {
namespace Core {
typedef Windows::SysDomainBase SysDomainBase;
}
}
#else
#error Unknown platform.
#endif

namespace Nirvana {
namespace Core {

class SysDomain :
	private SysDomainBase
{
public:
	static unsigned int hardware_concurrency ()
	{
		return SysDomainBase::hardware_concurrency ();
	}
};

}
}

#endif
