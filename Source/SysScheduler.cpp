#include "SysScheduler.h"
#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

AtomicCounter SysScheduler::activity_cnt_ = 0;
std::atomic <SysScheduler::State> SysScheduler::state_ = RUNNING;

void SysScheduler::run (Runnable_ptr startup, DeadlineTime deadline)
{
	ExecDomain::initialize ();
	Port::Scheduler::run (startup, deadline);
	ExecDomain::terminate ();
}

}
}
