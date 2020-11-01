#include "Thread.h"

namespace Nirvana {
namespace Core {

void Thread::enter_to (SyncDomain* sync_domain, bool ret)
{
	assert (exec_domain_);
	schedule_domain_ = sync_domain;
	schedule_ret_ = ret;
	CORBA::Nirvana::Environment env;
	neutral_context_.run_in_context (*this, &env);
	env.check ();
}

}
}
