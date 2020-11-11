#ifndef NIRVANA_CORE_RWLOCK_H_
#define NIRVANA_CORE_RWLOCK_H_

#include "AtomicCounter.h"
#include <atomic>

namespace Nirvana {
namespace Core {

class RWLock
{
public:
	RWLock ();

	void enter_read ();

	void leave_read ()
	{
		if (0 == read_cnt_.decrement ())
			write_lock_.clear ();
	}

	void enter_write ();

	void leave_write ()
	{
		write_lock_.clear ();
	}

	class GuardRead
	{
	public:
		GuardRead (RWLock& lock) :
			lock_ (lock)
		{
			lock.enter_read ();
		}

		~GuardRead ()
		{
			lock_.leave_read ();
		}

	private:
		RWLock& lock_;
	};

	class GuardWrite
	{
	public:
		GuardWrite (RWLock& lock) :
			lock_ (lock)
		{
			lock.enter_write ();
		}

		~GuardWrite ()
		{
			lock_.leave_write ();
		}

	private:
		RWLock& lock_;
	};

private:
	AtomicCounter <false> read_cnt_;
	std::atomic_flag write_lock_;
	volatile bool writer_waits_;
};

}
}

#endif
