#ifndef NIRVANA_CORE_SYSSCHEDULER_H_
#define NIRVANA_CORE_SYSSCHEDULER_H_

#include "ExecDomain.h"
#include <Port/Scheduler.h>

namespace Nirvana {
namespace Core {

class SysScheduler
{
public:
	static void run (Runnable_ptr startup, DeadlineTime deadline)
	{
		ExecDomain::initialize ();
		Port::Scheduler::run (startup, deadline);
		ExecDomain::terminate ();
	}

	static Scheduler_ptr singleton ()
	{
		return Port::Scheduler::singleton ();
	}

	static void shutdown ()
	{
		Port::Scheduler::shutdown ();
	}
};

}
}

#endif
