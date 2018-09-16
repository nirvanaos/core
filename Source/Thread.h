#ifndef NIRVANA_CORE_THREAD_H_
#define NIRVANA_CORE_THREAD_H_

#include "core.h"
#include "ExecDomain.h"
#include "RandomGen.h"
#include "../Interface/Runnable.h"

#ifdef _WIN32
#include "Windows/ThreadWindows.h"
namespace Nirvana {
namespace Core {
typedef Windows::ThreadWindows ThreadBase;
}
}
#else
#error Unknown platform.
#endif

namespace Nirvana {
namespace Core {

class ExecDomain;

class Thread :
	public CoreObject,
	protected ThreadBase
{
public:
	/// Returns current thread.
	static Thread& current ()
	{
		Thread* p = static_cast <Thread*> (ThreadBase::current ());
		assert (p);
		return *p;
	}

	Thread () :
		ThreadBase (),
		rndgen_ (),
		exec_domain_ (nullptr)
	{}

	template <class P>
	Thread (P param) :
		ThreadBase (param),
		rndgen_ (),
		exec_domain_ (nullptr)
	{}

	/// This static method is called by the scheduler.
	static void execute (Executor_ptr executor, DeadlineTime deadline)
	{
		executor->execute (deadline);
	}

	/// Random number generator's accessor.
	RandomGen& rndgen ()
	{
		return rndgen_;
	}

	/// Returns special "neutral" execution context with own stack and CPU state.
	virtual ExecContext* neutral_context ()
	{
		assert (false);
		return nullptr;
	}

protected:
	/// Random number generator.
	RandomGen rndgen_;

	/// Pointer to the current execution domain.
	ExecDomain* exec_domain_;
};

}
}

#endif
