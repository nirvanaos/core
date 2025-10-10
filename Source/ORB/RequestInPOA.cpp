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
#include "RequestInPOA.h"
#include "RqHelper.h"
#include "../Scheduler.h"

namespace CORBA {
namespace Core {

RequestInPOA::RequestInPOA ()
{
	// RequestInPOA is an activity.
	// Even if the request is in queue and does not have an associated ExecDomain,
	// it must counted as an activity.
	Nirvana::Core::Scheduler::activity_begin ();
}

RequestInPOA::~RequestInPOA ()
{
	Nirvana::Core::Scheduler::activity_end ();
}

void RequestInPOA::set_exception (Exception&& e) noexcept
{
	try {
		Any a = RqHelper::exception2any (std::move (e));
		set_exception (a);
	} catch (...) {
		set_unknown_exception ();
	}
}

void RequestInPOA::set_unknown_exception () noexcept
{
	try {
		set_exception (UNKNOWN ());
	} catch (...) {}
}

}
}
