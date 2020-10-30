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

class Thread
{
	friend class Port::Thread;
public:
	/// Returns current thread.
	static Thread& current ()
	{
		Thread* p = Port::Thread::current ();
		assert (p);
		return *p;
	}

	Thread () :
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

	/// Returns special "neutral" execution context with own stack and CPU state.
	ExecContext& neutral_context ()
	{
		return neutral_context_;
	}

	/// Returns synchronization context.
	virtual SynchronizationContext& sync_context () = 0;

	/// Returns runtime support object.
	virtual RuntimeSupportImpl& runtime_support () = 0;

private:
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
