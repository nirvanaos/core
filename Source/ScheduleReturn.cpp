#include "ScheduleReturn.h"
#include "Thread.h"
#include "SyncContext.h"

namespace Nirvana {
namespace Core {

void ScheduleReturn::run ()
{
	Thread& thread = Thread::current ();
	ExecDomain* exec_domain = thread.exec_domain ();
	assert (exec_domain);
	sync_context_.schedule_return (*exec_domain);
	thread.yield ();
}

}
}
