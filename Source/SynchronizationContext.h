// Core synchronization context.
#ifndef NIRVANA_CORE_SYNCHRONIZATIONCONTEXT_H_
#define NIRVANA_CORE_SYNCHRONIZATIONCONTEXT_H_

#include "Runnable.h"

namespace Nirvana {
namespace Core {

/// Core synchronization context.
class SynchronizationContext :
	public CoreInterface
{
public:
	/// Returns current synchronization context
	/// Returns smart pointer to ensure that context won't be destroyed
	/// until will be released.
	static Core_var <SynchronizationContext> current ();

	/// Returns free synchronization context.
	static Core_var <SynchronizationContext> not_synchronized ();

	/// Enter to the synchronization domain.
	/// \param ret `true` on return to call source domain.
	///            If `true` then causes fatal on error.
	virtual void enter (bool ret) = 0;

	/// Call runnable in the new execution domain.
	/// Exceptions are logged and swallowed.
	virtual void async_call (Runnable& runnable, DeadlineTime deadline) = 0;

	/// Returns `false` if there is no synchronization domain.
	virtual bool synchronized () = 0;

	/// Returns heap reference.
	virtual Heap& memory () = 0;
};

}
}

#endif
