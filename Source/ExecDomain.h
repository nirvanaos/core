// Nirvana project.
// Execution domain (coroutine, fiber).

#ifndef NIRVANA_CORE_EXECDOMAIN_H_
#define NIRVANA_CORE_EXECDOMAIN_H_

#include "ExecContext.h"
#include "ObjectPool.h"
#include "../Interface/Scheduler.h"
#include <limits>

namespace Nirvana {
namespace Core {

class SyncDomain;

class ExecDomain :
	public ExecContext,
	public PoolableObject,
	public ::CORBA::Nirvana::Servant <ExecDomain, Executor>
{
public:
	static void async_call (Runnable_ptr runnable, DeadlineTime deadline, SyncDomain* sync_domain);

	DeadlineTime deadline () const
	{
		return deadline_;
	}

	void execute (DeadlineTime deadline)
	{
		ExecContext::switch_to ();
	}

	ExecDomain () :
		ExecContext (),
		deadline_ (std::numeric_limits <DeadlineTime>::max ()),
		cur_sync_domain_ (nullptr)
	{}

	template <class P>
	ExecDomain (P param) :
		ExecContext (param),
		deadline_ (std::numeric_limits <DeadlineTime>::max ()),
		cur_sync_domain_ (nullptr)
	{}

private:
	static ObjectPoolT <ExecDomain> pool_;

	DeadlineTime deadline_;
	SyncDomain* cur_sync_domain_;
};

}
}

#endif
