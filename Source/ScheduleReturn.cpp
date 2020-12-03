#include "ScheduleReturn.h"
#include "Thread.h"
#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

void ScheduleReturn::run ()
{
	Thread& thread = Thread::current ();
	ExecDomain* exec_domain = thread.exec_domain ();
	assert (exec_domain);
	Core_var <SyncDomain> old_sync_domain = exec_domain->sync_context ()->sync_domain ();
	sync_context_.schedule_return (*exec_domain);
	if (old_sync_domain)
		old_sync_domain->leave ();
	thread.yield ();
}

}
}
