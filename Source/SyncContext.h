// Core synchronization context.
#ifndef NIRVANA_CORE_SYNCCONTEXT_H_
#define NIRVANA_CORE_SYNCCONTEXT_H_

#include "Thread.h"

namespace Nirvana {
namespace Core {

class Heap;
class ExecDomain;

/// Core synchronization context.
class NIRVANA_NOVTABLE SyncContext :
	public CoreInterface
{
public:
	/// Returns current synchronization context
	static SyncContext& current () NIRVANA_NOEXCEPT;

	/// Returns free synchronization context.
	static SyncContext& free_sync_context () NIRVANA_NOEXCEPT;

	static SyncDomain* SUSPEND ()
	{
		return (SyncDomain*)(~(uintptr_t)0);
	}

	/// Leave this context and enter to the synchronization domain.
	virtual void schedule_call (SyncDomain* sync_domain) = 0;

	/// Return execution domain to this context.
	virtual void schedule_return (ExecDomain& exec_domain) NIRVANA_NOEXCEPT = 0;

	/// Returns `nullptr` if it is free sync context.
	virtual SyncDomain* sync_domain () NIRVANA_NOEXCEPT = 0;

	/// May be used for proxy optimization.
	/// When we marshal `in` parameters from free context we haven't to copy data
	/// because all data are in stack or the execution domain heap and can not be changed
	/// by other threads during the synchronouse call.
	bool is_free_sync_context () const NIRVANA_NOEXCEPT
	{
		return this == &free_sync_context ();
	}

	/// Returns heap reference.
	virtual Heap& memory () NIRVANA_NOEXCEPT = 0;

protected:
	void check_schedule_error ();
};

}
}

#endif
