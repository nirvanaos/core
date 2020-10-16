#include "SynchronizationContext.h"
#include "SyncDomain.h"

namespace Nirvana {
namespace Core {

static class FreeSyncContext : 
	public ImplStatic <SynchronizationContext>
{
public:
	virtual void enter (bool ret);
	virtual void async_call (Runnable& runnable, DeadlineTime deadline);
	virtual bool synchronized ();
	virtual Heap& memory ();
} free_sync_context;

Core_var <SynchronizationContext> SynchronizationContext::current ()
{
	Thread& th = Thread::current ();
	SyncDomain* sd = th.execution_domain ()->cur_sync_domain ();
	SynchronizationContext* sc;
	if (sd)
		sc = sd;
	else
		sc = &free_sync_context;
	sc->_add_ref ();
	return sc;
}

void FreeSyncContext::enter (bool ret)
{
	Thread::current ().execution_domain ()->schedule (nullptr);
}

void FreeSyncContext::async_call (Runnable& runnable, DeadlineTime deadline)
{
	ExecDomain::async_call (runnable, deadline, nullptr);
}

bool FreeSyncContext::synchronized ()
{
	return false;
}

Heap& FreeSyncContext::memory ()
{
	Thread& th = Thread::current ();
	return th.execution_domain ()->heap ();
}

}
}
