#include "ThreadBackground.h"
#include "../ExecDomain.h"

namespace Nirvana {
namespace Legacy {
namespace Core {

using namespace Nirvana::Core;

void ThreadBackground::start (Nirvana::Core::ExecDomain& ed)
{
	exec_domain (&ed);
	ed.start ([this]() {this->create (); }, INFINITE_DEADLINE, nullptr);
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

::Nirvana::Core::SyncDomain* ThreadBackground::sync_domain ()
{
	assert (false);
	return nullptr;
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
