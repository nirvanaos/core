// Nirvana project.
// Execution domain (fiber).

#ifndef NIRVANA_CORE_EXECDOMAIN_H_
#define NIRVANA_CORE_EXECDOMAIN_H_

#include "core.h"
#include "PriorityQueue.h"
#include "Windows/ExecDomainBase.h"

namespace Nirvana {
namespace Core {

#ifdef _WIN32
typedef Windows::ExecDomainBase ExecDomainBase;
#else
#error Unknown platform.
#endif

class ExecDomain :
	private ExecDomainBase
{
public:
	DeadlineTime deadline () const
	{
		return deadline_;
	}

	PriorityQueue::RandomGen& rndgen ()
	{
		return rndgen_;
	}

	void switch_to ()
	{
		ExecDomainBase::switch_to ();
	}

private:
	PriorityQueue::RandomGen rndgen_;
	DeadlineTime deadline_;
};

}
}

#endif
