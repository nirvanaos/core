// Nirvana project.
// Execution domain (coroutine, fiber).

#ifndef NIRVANA_CORE_EXECDOMAIN_H_
#define NIRVANA_CORE_EXECDOMAIN_H_

#include "core.h"
#include "ObjectPool.h"
#include "../Interface/Scheduler.h"
#include "../Interface/Runnable.h"
#include <limits>

#ifdef _WIN32
#include "Windows/ExecContextWindows.h"
namespace Nirvana {
namespace Core {
typedef Windows::ExecContextWindows ExecContextBase;
}
}
#else
#error Unknown platform.
#endif

namespace Nirvana {
namespace Core {

class ExecContext :
	public CoreObject,
	protected ExecContextBase
{
public:
	ExecContext () :
		ExecContextBase ()
	{}

	template <class P>
	ExecContext (P param) :
		ExecContextBase (param)
	{}

private:
	Runnable_ptr runnable_;
};

class ExecDomain :
	public ExecContext,
	public PoolableObject,
	public ::CORBA::Nirvana::Servant <ExecDomain, Executor>
{
public:
	ExecDomain () :
		ExecContext (),
		deadline_ (std::numeric_limits <DeadlineTime>::max ())
	{}

	template <class P>
	ExecDomain (P param) :
		ExecContext (param),
		deadline_ (std::numeric_limits <DeadlineTime>::max ())
	{}

	DeadlineTime deadline () const
	{
		return deadline_;
	}

	void execute (DeadlineTime deadline)
	{
		ExecContextBase::switch_to ();
  }

private:
	DeadlineTime deadline_;
};

}
}

#endif
