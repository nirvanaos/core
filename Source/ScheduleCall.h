#ifndef NIRVANA_CORE_SCHEDULECALL_H_
#define NIRVANA_CORE_SCHEDULECALL_H_

#include "Runnable.h"

namespace Nirvana {
namespace Core {

class SyncDomain;

class NIRVANA_NOVTABLE ScheduleCall :
	private Runnable
{
public:
	static void schedule_call (SyncDomain* sync_domain);

protected:
	ScheduleCall (SyncDomain* sync_domain) NIRVANA_NOEXCEPT :
		sync_domain_ (sync_domain)
	{}

private:
	virtual void run ();
	virtual void on_exception () NIRVANA_NOEXCEPT;

private:
	SyncDomain* sync_domain_;
	std::exception_ptr exception_;
};

}
}

#endif
