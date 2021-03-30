/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
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
#ifndef NIRVANA_CORE_SCHEDULECALL_H_
#define NIRVANA_CORE_SCHEDULECALL_H_

#include "Runnable.h"

namespace Nirvana {
namespace Core {

class SyncDomain;

class ScheduleCall :
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
