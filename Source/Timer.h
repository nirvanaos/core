/// \file
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
#ifndef NIRVANA_CORE_TIMER_H_
#define NIRVANA_CORE_TIMER_H_
#pragma once

#include "Scheduler.h"
#include <Port/Timer.h>

namespace Nirvana {
namespace Core {

class NIRVANA_NOVTABLE Timer : 
	private Port::Timer
{
public:
	/// \brief Interpret initial expiration time as absolute UTC.
	static const unsigned TIMER_ABSOLUTE = Port::Timer::TIMER_ABSOLUTE;

	/// \brief Sets the timer object, replacing the previous timer, if any.
	/// 
	/// \param flags Flags.
	///
	/// \param initial Initial expiration.
	///   If TIMER_ABSOLUTE flag is specified, initial is interpreted as absolute UTC value.
	/// 
	/// \param period The timer period.
	///   If this parameter is zero, the timer is signaled once.
	///   If this parameter is greater than zero, the timer is periodic.
	///   A periodic timer automatically reactivates each time the period elapses,
	///   until the timer is canceled.
	void set (unsigned flags, TimeBase::TimeT initial, TimeBase::TimeT period)
	{
		periodic_ = !period;
		Port::Timer::set (flags, initial, period);
	}

	/// Disarm the timer.
	void cancel () noexcept
	{
		Port::Timer::cancel ();
	}

	/// Initialize timers.
	///
	/// Called on system startup.
	static void initialize ()
	{
		Port::Timer::initialize ();
	}

	/// Terminate all timers.
	///
	/// Called on system shutdown.
	static void terminate () noexcept
	{
		Port::Timer::terminate ();
	}

protected:
	Timer ()
	{}

	~Timer ()
	{}

	/// Signal timer.
	/// Called from the kernel thread.
	virtual void signal () noexcept = 0;

private:
	friend class Port::Timer;

	void port_signal () noexcept
	{
		// If shutdown is initiated, all periodic timers are disabled
		if (!periodic_ || Scheduler::state () == Scheduler::RUNNING)
			signal ();
	}

private:
	bool periodic_;
};

}
}

#endif
