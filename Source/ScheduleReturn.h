#ifndef NIRVANA_CORE_SCHEDULERETURN_H_
#define NIRVANA_CORE_SCHEDULERETURN_H_

#include "Runnable.h"

namespace Nirvana {
namespace Core {

class SyncContext;

class NIRVANA_NOVTABLE ScheduleReturn :
	private Runnable
{
public:
	static void schedule_return (SyncContext& sync_context) NIRVANA_NOEXCEPT
	{
		ImplStatic <ScheduleReturn> runnable (std::ref (sync_context));
		run_in_neutral_context (runnable);
	}

protected:
	ScheduleReturn (SyncContext& sync_context) NIRVANA_NOEXCEPT :
		sync_context_ (sync_context)
	{}

private:
	virtual void run ();

private:
	SyncContext& sync_context_;
};

}
}

#endif
