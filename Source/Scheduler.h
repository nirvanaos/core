#ifndef NIRVANA_CORE_SCHEDULER_H_
#define NIRVANA_CORE_SCHEDULER_H_

#include <Port/Scheduler.h>
#include "AtomicCounter.h"
#include <atomic>

namespace Nirvana {
namespace Core {

class Scheduler : public Port::Scheduler
{
public:
	static void shutdown () NIRVANA_NOEXCEPT;
	static void activity_begin ();
	static void activity_end () NIRVANA_NOEXCEPT;

private:
	static void check_shutdown () NIRVANA_NOEXCEPT;

private:
	enum class State
	{
		RUNNING = 0,
		SHUTDOWN_STARTED,
		SHUTDOWN_FINISH
	};

	static std::atomic <State> state_;
	static AtomicCounter <false> activity_cnt_;
};

}
}

#endif
