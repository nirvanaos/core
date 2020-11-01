#include "ThreadBackground.h"
#include "../ExecDomain.h"

namespace Nirvana {
namespace Legacy {
namespace Core {

using namespace Nirvana::Core;

void ThreadBackground::start (Nirvana::Core::ExecDomain& ed,
	Nirvana::Core::Runnable& runnable, CORBA::Nirvana::Interface* environment)
{
	exec_domain (&ed);
	ed.start (runnable, INFINITE_DEADLINE, nullptr, environment, [this]() {this->create (); });
	_add_ref ();
}

Nirvana::Core::SyncContext& ThreadBackground::sync_context () NIRVANA_NOEXCEPT
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

void ThreadBackground::enter_to (Nirvana::Core::SyncDomain* sync_domain, bool ret)
{
	if (sync_domain) {
		assert (!ret);
		Nirvana::Core::Thread::enter_to (sync_domain, ret);
	}
}

void ThreadBackground::run ()
{
	Nirvana::Core::Thread::run ();
	suspend ();
}


}
}
}
