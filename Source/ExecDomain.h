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
	public CoreObject,
	private ExecDomainBase
{
public:
	ExecDomain () :
		ExecDomainBase ()
	{}

	template <class P>
	ExecDomain (P param) :
		ExecDomainBase (param)
	{}

	/// Returns current execution domain.
	/// Can return nullptr if current thread executes in the special "neutral" context.
	static ExecDomain* current ()
	{
		return static_cast <ExecDomain*> (ExecDomainBase::current ());
	}

	DeadlineTime deadline () const
	{
		return deadline_;
	}

	void switch_to ()
	{
		ExecDomainBase::switch_to ();
	}

private:
	DeadlineTime deadline_;
};

}
}

#endif
