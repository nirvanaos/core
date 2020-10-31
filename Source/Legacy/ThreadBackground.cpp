#include "ThreadBackground.h"
#include "../ExecDomain.h"

namespace Nirvana {
namespace Legacy {
namespace Core {

using namespace Nirvana::Core;

void ThreadBackground::start (Nirvana::Core::ExecDomain& exec_domain,
	Nirvana::Core::Runnable& runnable, CORBA::Nirvana::Interface* environment)
{
	execution_domain (&exec_domain);
	exec_domain.start (runnable, INFINITE_DEADLINE, nullptr, environment, [this]() {this->create (); });
	_add_ref ();
}

Nirvana::Core::SynchronizationContext& ThreadBackground::sync_context ()
{
	return *this;
}

void ThreadBackground::enter (bool ret)
{
	assert (ret);
	resume ();
}

void ThreadBackground::async_call (Runnable& runnable, DeadlineTime deadline, CORBA::Nirvana::Interface_ptr environment)
{
	assert (false);
}

bool ThreadBackground::is_free_sync_context ()
{
	return false;
}

}
}
}
