#ifndef NIRVANA_CORE_THREAD_INL_
#define NIRVANA_CORE_THREAD_INL_

#include "Thread.h"
#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

inline void Thread::run ()
{
	exec_domain ()->schedule (schedule_domain_, schedule_ret_);
}

}
}

#endif
