#include "Scheduler.h"
#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

AtomicCounter Scheduler::activity_cnt_ = 0;
std::atomic <Scheduler::State> Scheduler::state_ = RUNNING;

void Scheduler::initialize ()
{
	ExecDomain::initialize ();
}

void Scheduler::terminate ()
{
	ExecDomain::initialize ();
}

}
}