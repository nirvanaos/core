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

class SyncDomain;
class SyncContext;
class RuntimeSupportImpl;

class NIRVANA_NOVTABLE Thread :
	protected Runnable // Runnable::run () is used for schedule.
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

	void exec_domain (ExecDomain* d) NIRVANA_NOEXCEPT
	{
		exec_domain_ = d;
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

	/// Returns synchronization context.
	virtual SyncContext& sync_context () NIRVANA_NOEXCEPT = 0;

	/// Returns runtime support object.
	virtual RuntimeSupportImpl& runtime_support () NIRVANA_NOEXCEPT = 0;

	/// Enter to a synchronization domain.
	/// Schedules current execution domain to execute in the synchronization domain.
	/// Does not throw an exception if `ret = true`.
	/// 
	/// \param sync_domain Synchronization domain. May be `nullptr`.
	/// \param ret `false` for call, `true` for return.
	virtual void enter_to (SyncDomain* sync_domain, bool ret);

protected:
	Thread () :
		exec_domain_ (nullptr),
		exec_context_ (&neutral_context_),
		neutral_context_ (true)
	{}

	virtual void run ()
	{
		exec_domain ()->schedule (schedule_domain_, schedule_ret_);
	}

private:
	/// Pointer to the current execution domain.
	ExecDomain* exec_domain_;

	/// Pointer to the current execution context.
	ExecContext* exec_context_;

	/// Special "neutral" execution context with own stack and CPU state.
	ExecContext neutral_context_;

	///@{
	/// Data for schedule.
	SyncDomain* schedule_domain_;
	bool schedule_ret_;
	///@}
};

}
}

#endif
