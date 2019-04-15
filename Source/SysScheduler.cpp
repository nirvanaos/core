#include "SysScheduler.h"
#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

AtomicCounter SysScheduler::activity_cnt_ = 0;
std::atomic <SysScheduler::State> SysScheduler::state_ = RUNNING;

void SysScheduler::initialize ()
{
	ExecDomain::initialize ();
}

void SysScheduler::terminate ()
{
	ExecDomain::initialize ();
}

}
}
