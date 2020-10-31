#ifndef NIRVANA_CORE_THREADWORKER_H_
#define NIRVANA_CORE_THREADWORKER_H_

#include "Thread.h"
#include <Port/ThreadWorker.h>

namespace Nirvana {
namespace Core {

class Executor;

class ThreadWorker :
	public Thread,
	private Port::ThreadWorker
{
	friend class Port::ThreadWorker;
public:
	/// Implementation - specific methods must be called explicitly.
	Port::ThreadWorker& port ()
	{
		return *this;
	}

	/// ThreadWorker instances are never created by core.
	/// The port implementation creates a pool of ThreadWorker objects on startup.
	template <class ... Args>
	ThreadWorker (Args ... args) :
		Port::ThreadWorker (std::forward <Args> (args)...)
	{}

		/// This static method is called by the scheduler.
	static void execute (Executor& executor, DeadlineTime deadline);

	/// Returns synchronization context.
	virtual SyncContext& sync_context ();

	/// Returns runtime support object.
	virtual RuntimeSupportImpl& runtime_support ();
};

}
}

#endif
