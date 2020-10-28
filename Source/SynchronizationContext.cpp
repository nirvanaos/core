#include "SynchronizationContext.h"
#include "Thread.h"
#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

class FreeSyncContext :
	public SynchronizationContext
{
public:
	virtual void enter (bool ret);
	virtual void async_call (Runnable& runnable, DeadlineTime deadline, CORBA::Nirvana::EnvironmentBridge* environment);
	virtual bool is_free_sync_context ();
	virtual Heap& memory ();
};

ImplStatic <FreeSyncContext> g_free_sync_context;

SynchronizationContext& SynchronizationContext::free_sync_context ()
{
	return g_free_sync_context;
}

void FreeSyncContext::enter (bool ret)
{
	Thread::current ().execution_domain ()->schedule (nullptr);
}

void FreeSyncContext::async_call (Runnable& runnable, DeadlineTime deadline, CORBA::Nirvana::EnvironmentBridge* environment)
{
	ExecDomain::async_call (runnable, deadline, nullptr, environment);
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
