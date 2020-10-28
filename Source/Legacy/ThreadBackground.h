#ifndef NIRVANA_CORE_THREADBACKGROUND_H_
#define NIRVANA_CORE_THREADBACKGROUND_H_

#include "../Thread.h"
#include "../SynchronizationContext.h"
#include <Port/ThreadBackground.h>

namespace Nirvana {
namespace Legacy {
namespace Core {

/// Background thread.
/// Used in the legacy mode implementation.
class ThreadBackground :
	public Nirvana::Core::Port::ThreadBackground,
	public Nirvana::Core::SynchronizationContext
{
public:
	ThreadBackground (Nirvana::Core::Runnable& runnable, CORBA::Nirvana::Interface* environment);

private:
	friend class Nirvana::Core::Port::ThreadBackground;

	void on_thread_proc_end ()
	{
		_remove_ref ();
	}
};

}
}
}

#endif
