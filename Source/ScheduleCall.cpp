#include "ScheduleCall.h"
#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

void ScheduleCall::schedule_call (SyncDomain* sync_domain)
{
	ScheduleCall runnable (sync_domain);
	run_in_neutral_context (runnable);
	if (runnable.exception_)
		std::rethrow_exception (runnable.exception_);
}

void ScheduleCall::run ()
{
	ExecDomain* exec_domain = Thread::current ().exec_domain ();
	assert (exec_domain);
	exec_domain->schedule (sync_domain_);
	Thread::current ().yield ();
}

void ScheduleCall::on_exception () NIRVANA_NOEXCEPT
{
	exception_ = std::current_exception ();
}

}
}
