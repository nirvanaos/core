// The Nirvana project.
// Core.
// Thread class.
#ifndef NIRVANA_CORE_THREAD_H_
#define NIRVANA_CORE_THREAD_H_

#include "core.h"
#include "RandomGen.h"
#include <Nirvana/Runnable_c.h>

#ifdef _WIN32
#include "ThreadWindows.h"
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

class ExecContext;
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

	ExecDomain* execution_domain () const
	{
		return exec_domain_;
	}

	/// This static method is called by the scheduler.
	static void execute (Executor_ptr executor, DeadlineTime deadline);

	/// Random number generator's accessor.
	RandomGen& rndgen ()
	{
		return rndgen_;
	}

	/// Returns special "neutral" execution context with own stack and CPU state.
	ExecContext* neutral_context ()
	{
		return ThreadBase::neutral_context ();
	}
	
	/// Temporary boost the priority for time-critical operations.
	/// \param boost `true` - raise priority above worker thread, `false` - down priority to background.
	virtual void boost_priority (bool boost)
	{} /// For worker thread does nothing. Overridden in ThreadBackground.

private:
	friend class ExecDomain;

	void execution_domain (ExecDomain* d)
	{
		exec_domain_ = d;
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
