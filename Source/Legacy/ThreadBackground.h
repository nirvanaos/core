#ifndef NIRVANA_LEGACY_CORE_THREADBACKGROUND_H_
#define NIRVANA_LEGACY_CORE_THREADBACKGROUND_H_

#include "../Thread.h"
#include "../SyncContext.h"
#include <Port/ThreadBackground.h>

namespace Nirvana {

namespace Core {
class RuntimeSupportImpl;
}

namespace Legacy {
namespace Core {

/// Background thread.
/// Used in the legacy mode implementation.
class NIRVANA_NOVTABLE ThreadBackground :
	public Nirvana::Core::Thread,
	public Nirvana::Core::SyncContext,
	private Nirvana::Core::Port::ThreadBackground
{
	friend class Nirvana::Core::Port::ThreadBackground;
public:
	/// Implementation - specific methods must be called explicitly.
	Nirvana::Core::Port::ThreadBackground& port ()
	{
		return *this;
	}

	/// When we run in the legacy subsystem, every thread is a ThreadBackground instance.
	static ThreadBackground& current () NIRVANA_NOEXCEPT
	{
		return static_cast <ThreadBackground&> (Nirvana::Core::Thread::current ());
	}

	/// Returns synchronization context.
	virtual Nirvana::Core::SyncContext& sync_context () NIRVANA_NOEXCEPT;

	virtual void enter (bool ret);
	virtual void async_call (Runnable& runnable, DeadlineTime deadline, CORBA::Nirvana::Interface_ptr environment);
	virtual bool is_free_sync_context ();

	virtual void enter_to (Nirvana::Core::SyncDomain* sync_domain, bool ret);

protected:
	void start (Nirvana::Core::ExecDomain& ed, 
		Nirvana::Core::Runnable& runnable, CORBA::Nirvana::Interface* environment);

	virtual void _add_ref () = 0;
	virtual void _remove_ref () = 0;

private:
	virtual void run ();

	void on_thread_proc_end ()
	{
		_remove_ref ();
	}
};

}
}
}

#endif
