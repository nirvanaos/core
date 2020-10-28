// Nirvana project.
// System domain.
#ifndef NIRVANA_CORE_SYSDOMAIN_H_
#define NIRVANA_CORE_SYSDOMAIN_H_

#include "CoreObject.h"
#include <Port/SysDomain.h>

namespace Nirvana {
namespace Core {

class SysDomain :
	public CoreObject,
	private Port::SysDomain
{
public:
	Port::SysDomain& port ()
	{
		return *this;
	}

	class ProtDomainInfo :
		private Port::SysDomain::ProtDomainInfo
	{
	public:
		Port::SysDomain::ProtDomainInfo& port ()
		{
			return *this;
		}

		template <typename T>
		ProtDomainInfo (T p) :
			Port::SysDomain::ProtDomainInfo (p)
		{}
	};
};

}
}

#endif
