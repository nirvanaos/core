#include "SyncContext.h"
#include "Thread.h"
#include "ExecDomain.h"
#include "ScheduleCall.h"
#include "ScheduleReturn.h"
#include "Suspend.h"

namespace Nirvana {
namespace Core {

class NIRVANA_NOVTABLE FreeSyncContext :
	public SyncContext
{
public:
	virtual void schedule_call (SyncDomain* sync_domain);
	virtual void schedule_return (ExecDomain& exec_domain) NIRVANA_NOEXCEPT;
	virtual SyncDomain* sync_domain () NIRVANA_NOEXCEPT;
	virtual Heap& memory () NIRVANA_NOEXCEPT;
};

ImplStatic <FreeSyncContext> g_free_sync_context;

SyncContext& SyncContext::current () NIRVANA_NOEXCEPT
{
	ExecDomain* ed = Thread::current ().exec_domain ();
	assert (ed);
	SyncContext* sc = ed->sync_context ();
	assert (sc);
	return *sc;
}

SyncContext& SyncContext::free_sync_context () NIRVANA_NOEXCEPT
{
	return g_free_sync_context;
}

void SyncContext::check_schedule_error ()
{
	CORBA::Exception::Code err = Thread::current ().exec_domain ()->scheduler_error ();
	if (err) {
		// We must return to prev synchronization domain back before throwing the exception.
		ScheduleReturn::schedule_return (*this);
		CORBA::SystemException::_raise_by_code (err);
	}
}

void FreeSyncContext::schedule_call (SyncDomain* sync_domain)
{
	if (SyncContext::SUSPEND () == sync_domain)
		Suspend::suspend ();
	else {
		ScheduleCall::schedule_call (sync_domain);
		check_schedule_error ();
	}
}

void FreeSyncContext::schedule_return (ExecDomain& exec_domain) NIRVANA_NOEXCEPT
{
	exec_domain.sync_context (*this);
	Scheduler::schedule (exec_domain.deadline (), exec_domain);
}

SyncDomain* FreeSyncContext::sync_domain () NIRVANA_NOEXCEPT
{
	return nullptr;
}

Heap& FreeSyncContext::memory () NIRVANA_NOEXCEPT
{
	Thread& th = Thread::current ();
	return th.exec_domain ()->heap ();
}

}
}
