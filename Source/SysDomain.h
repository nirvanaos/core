// Nirvana project.
// System domain.
#ifndef NIRVANA_CORE_SYSDOMAIN_H_
#define NIRVANA_CORE_SYSDOMAIN_H_

#include "core.h"

#ifdef _WIN32
#include "Windows/SysDomainWindows.h"
namespace Nirvana {
namespace Core {
typedef Windows::SysDomainWindows SysDomainBase;
}
}
#else
#error Unknown platform.
#endif

namespace Nirvana {
namespace Core {

class SysDomain :
	public CoreObject,
	private SysDomainBase
{
public:
	class ProtDomainInfo :
		public SysDomainBase::ProtDomainInfo
	{
	public:
		template <typename T>
		ProtDomainInfo (T p) :
			SysDomainBase::ProtDomainInfo (p)
		{}
	};
};

}
}

#endif
