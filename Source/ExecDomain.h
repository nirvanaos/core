// Nirvana project.
// Execution domain (fiber).

#ifndef NIRVANA_CORE_EXECDOMAIN_H_
#define NIRVANA_CORE_EXECDOMAIN_H_

#include "core.h"
#include "PriorityQueue.h"

#ifdef _WIN32
#include "Windows/ExecDomainBase.h"
namespace Nirvana {
namespace Core {
typedef Windows::ExecDomainBase ExecDomainBase;
}
}
#else
#error Unknown platform.
#endif

namespace Nirvana {
namespace Core {

class ExecDomain :
	private ExecDomainBase
{
public:
	DeadlineTime deadline () const
	{
		return deadline_;
	}

	RandomGen& rndgen ()
	{
		return rndgen_;
	}

	void switch_to ()
	{
		ExecDomainBase::switch_to ();
	}

private:
	RandomGen rndgen_;
	DeadlineTime deadline_;
};

}
}

#endif
