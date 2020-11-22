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
	protected Port::Thread,
	public SpinLockNode
{
	friend class Port::Thread;
public:
	// Implementation - specific methods can be called explicitly.
	Port::Thread& port () NIRVANA_NOEXCEPT
	{
		return *this;
	}

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
		neutral_context_ (true)
	{}

protected:
	RuntimeSupportImpl* runtime_support_;

private:
	/// Pointer to the current execution domain.
	ExecDomain* exec_domain_;

	/// Special "neutral" execution context with own stack and CPU state.
	ExecContext neutral_context_;
};

}
}

#endif
