// Core synchronization context.
#ifndef NIRVANA_CORE_SYNCHRONIZATIONCONTEXT_H_
#define NIRVANA_CORE_SYNCHRONIZATIONCONTEXT_H_

#include "Thread.h"

namespace Nirvana {
namespace Core {

class Runnable;
class Heap;

/// Core synchronization context.
class NIRVANA_NOVTABLE SyncContext :
	public CoreInterface
{
public:
	/// Returns current synchronization context
	static SyncContext& current ()
	{
		return Thread::current ().sync_context ();
	}

	/// Returns free synchronization context.
	static SyncContext& free_sync_context ();

	/// Enter to the synchronization domain.
	/// \param ret `true` on return to call source domain.
	///            If `true` then causes fatal on error.
	virtual void enter (bool ret) = 0;

	/// Returns `nullptr` if it is free sync context.
	/// May be used for proxy optimization.
	/// When we marshal `in` parameters from free context we haven't to copy data.
	virtual SyncDomain* sync_domain () = 0;

	/// Returns heap reference.
	virtual Heap& memory () = 0;
};

}
}

#endif
