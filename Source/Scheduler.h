/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2021 Igor Popov.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*
* Send comments and/or bug reports to:
*  popov.nirvana@gmail.com
*/
#ifndef NIRVANA_CORE_SCHEDULER_H_
#define NIRVANA_CORE_SCHEDULER_H_
#pragma once

#include <Port/Scheduler.h>
#include "AtomicCounter.h"
#include "Runnable.h"
#include <atomic>
#include "StaticallyAllocated.h"

namespace Nirvana {
namespace Core {

class Scheduler
{
public:
	enum State
	{
		RUNNING = 0,
		SHUTDOWN_STARTED,
		TERMINATE,
		SHUTDOWN_FINISH
	};

	static void initialize ()
	{
		global_.construct ();
	}

	static void terminate () NIRVANA_NOEXCEPT
	{
		//global_.destruct ();
	}

	static State state () NIRVANA_NOEXCEPT
	{
		return global_->state;
	}

	/// Start new activity.
	/// Called on creation of the execution domain.
	static void activity_begin () NIRVANA_NOEXCEPT
	{
		assert (global_->state < State::SHUTDOWN_FINISH);
		global_->activity_cnt.increment ();
	}

	/// End activity.
	/// Called on completion of the execution domain.
	static void activity_end () NIRVANA_NOEXCEPT;

	/// Initiates shutdown.
	/// Shutdown will be completed when activity count became zero.
	static void shutdown ();

	/// Reserve space for an active item.
	/// \throws CORBA::NO_MEMORY
	static void create_item ()
	{
		Port::Scheduler::create_item ();
	}

	/// Release active item space.
	static void delete_item () NIRVANA_NOEXCEPT
	{
		Port::Scheduler::delete_item ();
	}

	/// \summary Schedule execution.
	/// 
	/// \param deadline Deadline.
	/// \param executor Executor.
	static void schedule (const DeadlineTime& deadline, Executor& executor) NIRVANA_NOEXCEPT
	{
		Port::Scheduler::schedule (deadline, executor);
	}

	/// \summary Re-schedule execution.
	/// 
	/// \param deadline New deadline.
	/// \param executor Executor.
	/// \param old Old deadline.
	/// \returns `true` if the executor was found and rescheduled. `false` if executor with old deadline was not found.
	static bool reschedule (const DeadlineTime& deadline, Executor& executor, const DeadlineTime& old) NIRVANA_NOEXCEPT
	{
		return Port::Scheduler::reschedule (deadline, executor, old);
	}

private:
	static void do_shutdown ();

	class Shutdown : public Runnable
	{
	private:
		virtual void run ();
	};

	class Terminator : public Runnable
	{
	private:
		virtual void run ();
	};

	struct GlobalData
	{
		std::atomic <State> state;
		AtomicCounter <false> activity_cnt;

		GlobalData () :
			state (State::RUNNING),
			activity_cnt (0)
		{}
	};

	static StaticallyAllocated <GlobalData> global_;
};

}
}

#endif
