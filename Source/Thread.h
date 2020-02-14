// The Nirvana project.
// Core.
// Thread class.
#ifndef NIRVANA_CORE_THREAD_H_
#define NIRVANA_CORE_THREAD_H_

#include "core.h"
#include "RandomGen.h"
#include <Nirvana/Runnable.h>
#include <Port/Thread.h>
#include "Scheduler.h"

namespace Nirvana {
namespace Core {

class ExecContext;
class ExecDomain;

class Thread :
	public CoreObject,
	private Port::Thread
{
public:
	Port::Thread& port ()
	{
		return *this;
	}

	/// Returns current thread.
	static Thread& current ()
	{
		Thread* p = static_cast <Thread*> (Port::Thread::current ());
		assert (p);
		return *p;
	}

	Thread () :
		Port::Thread (),
		rndgen_ (),
		exec_domain_ (nullptr),
		exec_context_ (nullptr)
	{}

	template <class P>
	Thread (P param) :
		Port::Thread (param),
		rndgen_ (),
		exec_domain_ (nullptr),
		exec_context_ (nullptr)
	{}

	ExecDomain* execution_domain () const
	{
		return exec_domain_;
	}

	void execution_domain (ExecDomain* d)
	{
		exec_domain_ = d;
	}

	ExecContext* context () const
	{
		return exec_context_;
	}

	void context (ExecContext* c)
	{
		exec_context_ = c;
	}

	/// This static method is called by the scheduler.
	static void execute (Executor& executor, DeadlineTime deadline);

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

	/// Pointer to the current execution context.
	ExecContext* exec_context_;
};

}
}

#endif
