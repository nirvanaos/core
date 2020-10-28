// Core synchronization context.
#ifndef NIRVANA_CORE_SYNCHRONIZATIONCONTEXT_H_
#define NIRVANA_CORE_SYNCHRONIZATIONCONTEXT_H_

#include "Thread.h"

namespace Nirvana {
namespace Core {

class Runnable;

/// Core synchronization context.
class SynchronizationContext :
	public CoreInterface
{
public:
	/// Returns current synchronization context
	static SynchronizationContext& current ()
	{
		return Thread::current ().sync_context ();
	}

	/// Returns free synchronization context.
	static SynchronizationContext& free_sync_context ();

	/// Enter to the synchronization domain.
	/// \param ret `true` on return to call source domain.
	///            If `true` then causes fatal on error.
	virtual void enter (bool ret) = 0;

	/// Call runnable in the new execution domain.
	virtual void async_call (Runnable& runnable, DeadlineTime deadline, CORBA::Nirvana::EnvironmentBridge* environment = nullptr) = 0;

	/// Returns `true` if there is free sync context.
	/// May be used for proxy optimization.
	/// When we marshal `in` parameters from free context we haven't to copy data.
	virtual bool is_free_sync_context () = 0;

	/// Returns heap reference.
	virtual Heap& memory () = 0;
};

}
}

#endif
