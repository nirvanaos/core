#ifndef NIRVANA_LEGACY_CORE_THREADBACKGROUND_H_
#define NIRVANA_LEGACY_CORE_THREADBACKGROUND_H_

#include "../Thread.h"
#include "../SyncContext.h"
#include <Port/ThreadBackground.h>
#include "RuntimeSupportLegacy.h"

namespace Nirvana {

namespace Core {
class RuntimeSupportImpl;
}

namespace Legacy {
namespace Core {

/// Background thread.
/// Used in the legacy mode implementation.
class NIRVANA_NOVTABLE ThreadBackground :
	protected Nirvana::Core::Port::ThreadBackground,
	public Nirvana::Core::SyncContext
{
	friend class Nirvana::Core::Port::ThreadBackground;
public:
	/// Implementation - specific methods can be called explicitly.
	Nirvana::Core::Port::ThreadBackground& port ()
	{
		return *this;
	}

	/// When we run in the legacy subsystem, every thread is a ThreadBackground instance.
	static ThreadBackground& current () NIRVANA_NOEXCEPT
	{
		return static_cast <ThreadBackground&> (Nirvana::Core::Thread::current ());
	}

	/// Leave this context and enter to the synchronization domain.
	virtual void schedule_call (Nirvana::Core::SyncDomain* sync_domain);

	/// Return execution domain to this context.
	virtual void schedule_return (Nirvana::Core::ExecDomain& exec_domain) NIRVANA_NOEXCEPT;

	/// Returns `nullptr` if it is free sync context.
	/// May be used for proxy optimization.
	/// When we marshal `in` parameters from free context we haven't to copy data.
	virtual Nirvana::Core::SyncDomain* sync_domain () NIRVANA_NOEXCEPT;

	/// Returns heap reference.
	// virtual Nirvana::Core::Heap& memory () = 0;

	RuntimeSupportLegacy& runtime_support () const NIRVANA_NOEXCEPT
	{
		assert (runtime_support_);
		return static_cast <RuntimeSupportLegacy&> (*runtime_support_);
	}

	virtual void yield () NIRVANA_NOEXCEPT;

protected:
	void start (RuntimeSupportLegacy& runtime_support, Nirvana::Core::ExecDomain& execution_domain);

private:
	void on_thread_proc_end ()
	{
		_remove_ref ();
	}
};

}
}
}

#endif
