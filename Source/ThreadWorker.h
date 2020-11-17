#ifndef NIRVANA_CORE_THREADWORKER_H_
#define NIRVANA_CORE_THREADWORKER_H_

#include "Thread.h"

namespace Nirvana {
namespace Core {

class Executor;

/// ThreadWorker instances are never created by core.
/// The underlying port implementation creates a pool of ThreadWorker objects on startup.
class NIRVANA_NOVTABLE ThreadWorker :
	public Core::Thread
{
public:
	/// This static method is called by the scheduler.
	static void execute (Executor& executor, Word scheduler_error);

	virtual void yield () NIRVANA_NOEXCEPT;
};

}
}

#endif
