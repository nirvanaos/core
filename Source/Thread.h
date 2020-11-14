// The Nirvana project.
// Core.
// Thread class.
#ifndef NIRVANA_CORE_THREAD_H_
#define NIRVANA_CORE_THREAD_H_

#include "core.h"
#include "ExecContext.h"
#include "SpinLock.h"
#include <Port/Thread.h>

namespace Nirvana {
namespace Core {

class ExecDomain;
class SyncDomain;
class SyncContext;
class RuntimeSupportImpl;

class NIRVANA_NOVTABLE Thread :
	public SpinLockNode
{
public:
	/// Returns current thread.
	static Thread& current () NIRVANA_NOEXCEPT
	{
		Thread* p = Port::Thread::current ();
		assert (p);
		return *p;
	}

	ExecDomain* exec_domain () const NIRVANA_NOEXCEPT
	{
		return exec_domain_;
	}

	void exec_domain (ExecDomain& exec_domain) NIRVANA_NOEXCEPT;

	void exec_domain (nullptr_t) NIRVANA_NOEXCEPT
	{
		exec_domain_ = nullptr;
		runtime_support_ = nullptr;
	}

	ExecContext& context () const NIRVANA_NOEXCEPT
	{
		return *exec_context_;
	}

	void context (ExecContext& c) NIRVANA_NOEXCEPT
	{
		exec_context_ = &c;
	}

	/// Returns special "neutral" execution context with own stack and CPU state.
	ExecContext& neutral_context () NIRVANA_NOEXCEPT
	{
		return neutral_context_;
	}

	/// Returns runtime support object.
	RuntimeSupportImpl& runtime_support () const NIRVANA_NOEXCEPT
	{
		assert (runtime_support_);
		return *runtime_support_;
	}

	virtual void yield () NIRVANA_NOEXCEPT = 0;

protected:
	Thread () :
		exec_domain_ (nullptr),
		exec_context_ (&neutral_context_),
		neutral_context_ (true)
	{}

protected:
	RuntimeSupportImpl* runtime_support_;

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
