#include "ThreadBackground.h"
#include "../ExecDomain.h"
#include "../ScheduleCall.h"
#include "../Suspend.h"

namespace Nirvana {
namespace Legacy {
namespace Core {

using namespace Nirvana::Core;

void ThreadBackground::start (RuntimeSupportLegacy& runtime_support, Nirvana::Core::ExecDomain& execution_domain)
{
	exec_domain (execution_domain);
	runtime_support_ = &runtime_support;
	execution_domain.start ([this]() {this->create (); });
	_add_ref ();
}

::Nirvana::Core::SyncDomain* ThreadBackground::sync_domain () NIRVANA_NOEXCEPT
{
	return nullptr;
}

void ThreadBackground::schedule_call (Nirvana::Core::SyncDomain* sync_domain)
{
	// We don't switch context if sync_domain == nullptr
	if (sync_domain) {
		if (SyncContext::SUSPEND () == sync_domain)
			Suspend::suspend ();
		else {
			ScheduleCall::schedule_call (sync_domain);
			check_schedule_error ();
		}
	}
}

void ThreadBackground::schedule_return (Nirvana::Core::ExecDomain& exec_domain) NIRVANA_NOEXCEPT
{
	Port::ThreadBackground::resume ();
}

void ThreadBackground::yield () NIRVANA_NOEXCEPT
{
	Port::ThreadBackground::suspend ();
}

}
}
}
