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
	virtual bool is_free_sync_context ();
	virtual Heap& memory ();
} g_free_sync_context;

Core_var <SynchronizationContext> SynchronizationContext::current ()
{
	Thread& th = Thread::current ();
	SyncDomain* sd = th.execution_domain ()->cur_sync_domain ();
	SynchronizationContext* sc;
	if (sd)
		sc = sd;
	else
		sc = &g_free_sync_context;
	sc->_add_ref ();
	return sc;
}

Core_var <SynchronizationContext> SynchronizationContext::free_sync_context ()
{
	g_free_sync_context._add_ref ();
	return &g_free_sync_context;
}

void FreeSyncContext::enter (bool ret)
{
	Thread::current ().execution_domain ()->schedule (nullptr);
}

void FreeSyncContext::async_call (Runnable& runnable, DeadlineTime deadline)
{
	ExecDomain::async_call (runnable, deadline, nullptr);
}

bool FreeSyncContext::is_free_sync_context ()
{
	return true;
}

Heap& FreeSyncContext::memory ()
{
	Thread& th = Thread::current ();
	return th.execution_domain ()->heap ();
}

}
}
