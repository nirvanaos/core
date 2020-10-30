#ifndef NIRVANA_CORE_THREADBACKGROUND_H_
#define NIRVANA_CORE_THREADBACKGROUND_H_

#include "../Thread.h"
#include "../SynchronizationContext.h"
#include <Port/ThreadBackground.h>

namespace Nirvana {

namespace Core {
class RuntimeSupportImpl;
}

namespace Legacy {
namespace Core {

/// Background thread.
/// Used in the legacy mode implementation.
class ThreadBackground :
	public Nirvana::Core::Thread,
	public Nirvana::Core::SynchronizationContext,
	private Nirvana::Core::Port::ThreadBackground
{
	friend class Nirvana::Core::Port::ThreadBackground;
public:
	/// Implementation - specific methods must be called explicitly.
	Nirvana::Core::Port::ThreadBackground& port ()
	{
		return *this;
	}

	/// Returns synchronization context.
	virtual SynchronizationContext& sync_context ();

protected:
	void start (Nirvana::Core::ExecDomain& exec_domain, 
		Nirvana::Core::Runnable& runnable, CORBA::Nirvana::Interface* environment);

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
