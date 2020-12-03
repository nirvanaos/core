#ifndef NIRVANA_CORE_SYNCHRONIZED_H_
#define NIRVANA_CORE_SYNCHRONIZED_H_

#include "SyncContext.h"
#include "ScheduleReturn.h"

namespace Nirvana {
namespace Core {

class Synchronized
{
public:
	Synchronized (SyncDomain* target) :
		call_context_ (&SyncContext::current ())
	{
		call_context_->schedule_call (target);
	}

	Synchronized (SyncContext& target) :
		call_context_ (&SyncContext::current ())
	{
		SyncDomain* sync_domain = target.sync_domain ();
		assert (sync_domain || target.is_free_sync_context ());
		call_context_->schedule_call (sync_domain);
	}

	~Synchronized ()
	{
		if (call_context_)
			ScheduleReturn::schedule_return (*call_context_);
	}

	SyncContext& call_context () const
	{
		return *call_context_;
	}

	NIRVANA_NORETURN void on_exception ()
	{
		std::exception_ptr exc = std::current_exception ();
		Core_var <SyncContext> context = std::move (call_context_);
		ScheduleReturn::schedule_return (*context);
		std::rethrow_exception (exc);
	}

private:
	Core_var <SyncContext> call_context_;
};

}
}

#define SYNC_BEGIN(target) { ::Nirvana::Core::Synchronized sync (target); try {
#define SYNC_END() } catch (...) { sync.on_exception (); }}

#endif
