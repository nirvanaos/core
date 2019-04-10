// Nirvana project.
// System domain.
#ifndef NIRVANA_CORE_SYSDOMAIN_H_
#define NIRVANA_CORE_SYSDOMAIN_H_

#include "core.h"
#include "PortSysDomain.h"

namespace Nirvana {
namespace Core {

class SysDomain :
	public CoreObject,
	private Port::SysDomain
{
public:
	class ProtDomainInfo :
		public Port::SysDomain::ProtDomainInfo
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
