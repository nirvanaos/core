// Nirvana project.
// Protection domain.
#ifndef NIRVANA_CORE_PROTDOMAIN_H_
#define NIRVANA_CORE_PROTDOMAIN_H_

#include "SyncDomain.h"
#ifdef _WIN32
#include "Windows/ProtDomainBase.h"
namespace Nirvana {
namespace Core {
typedef Windows::ProtDomainBase ProtDomainBase;
}
}
#else
#error Unknown platform.
#endif

namespace Nirvana {
namespace Core {

class ProtDomain :
	private ProtDomainBase
{
public:
	static void schedule (SyncDomain& sd, bool update)
	{
		ProtDomainBase::schedule (sd.min_deadline (), &sd, update);
	}

private:
};

}
}

#endif
