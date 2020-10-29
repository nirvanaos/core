// The Nirvana project.
// Core.
// Thread class.
#ifndef NIRVANA_CORE_THREAD_H_
#define NIRVANA_CORE_THREAD_H_

#include "core.h"
#include <Port/Thread.h>
#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

class ExecContext;
class ExecDomain;
class SynchronizationContext;

class Thread :
	private Port::Thread
{
	friend class Port::Thread;
public:
	// Implementation - specific methods must be called explicitly.
	Port::Thread& port ()
	{
		return *this;
	}

	/// Returns current thread.
	static Thread& current ()
	{
		Thread* p = Port::Thread::current ();
		assert (p);
		return *p;
	}

	template <class ... Args>
	Thread (Args ... args) :
		Port::Thread (std::forward <Args> (args)...),
		exec_domain_ (nullptr),
		exec_context_ (&neutral_context_),
		neutral_context_ (true)
	{}

	ExecDomain* execution_domain () const
	{
		return exec_domain_;
	}

	void execution_domain (ExecDomain* d)
	{
		exec_domain_ = d;
	}

	ExecContext& context () const
	{
		return *exec_context_;
	}

	void context (ExecContext& c)
	{
		exec_context_ = &c;
	}

	/// This static method is called by the scheduler.
	static void execute (Executor& executor, DeadlineTime deadline);

	/// Returns special "neutral" execution context with own stack and CPU state.
	ExecContext& neutral_context ()
	{
		return neutral_context_;
	}

	/// Returns synchronization context.
	virtual SynchronizationContext& sync_context ();

	virtual RuntimeSupportImpl& runtime_support ();

protected:
	/// Pointer to the current execution domain.
	ExecDomain* exec_domain_;

	/// Pointer to the current execution context.
	ExecContext* exec_context_;

	/// Special "neutral" execution context with own stack and CPU state.
	ExecContext neutral_context_;
};

}
}

#endif
