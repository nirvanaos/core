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
#ifndef NIRVANA_CORE_TIMERSLEEP_H_
#define NIRVANA_CORE_TIMERSLEEP_H_
#pragma once

#include "Timer.h"
#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

/// @brief Used for POSIX sleep ()
class TimerSleep : public Timer
{
public:
	void sleep (const TimeBase::TimeT& t)
	{
		Thread& thread = Thread::current ();
		ExecDomain* ed = exec_domain_ = thread.exec_domain ();
		bool pop_qnode = ed->suspend_prepare_no_leave ();
		try {
			Timer::set (0, t, 0);
		} catch (...) {
			ed->suspend_unprepare (pop_qnode);
			throw;
		}
		Port::Thread::PriorityBoost boost;
		ed->leave_and_suspend ();
	}

private:
	virtual void signal () noexcept override
	{
		if (exec_domain_->suspended ())
			exec_domain_->resume ();
	}

private:
	ExecDomain* exec_domain_;
};

}
}

#endif
