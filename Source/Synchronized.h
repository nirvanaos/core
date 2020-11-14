#ifndef NIRVANA_CORE_SYNCHRONIZED_H_
#define NIRVANA_CORE_SYNCHRONIZED_H_

#include "SyncContext.h"
#include "ScheduleReturn.h"

namespace Nirvana {
namespace Core {

class Synchronized
{
public:
	Synchronized () :
		call_context_ (&SyncContext::current ())
	{}

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
		ScheduleReturn::schedule_return (*call_context_);
	}

	SyncContext& call_context () const
	{
		return *call_context_;
	}

private:
	Core_var <SyncContext> call_context_;
};

}
}

#endif
