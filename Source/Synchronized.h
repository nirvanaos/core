/// \file
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
#ifndef NIRVANA_CORE_SYNCHRONIZED_H_
#define NIRVANA_CORE_SYNCHRONIZED_H_

#include "SyncContext.h"
#include "ScheduleReturn.h"

namespace Nirvana {
namespace Core {

class Synchronized
{
public:
	Synchronized (SyncDomain* target) :
		call_context_ (&SyncContext::current ())
	{
		call_context_->schedule_call (target);
	}

	Synchronized (SyncContext& target) :
		call_context_ (&SyncContext::current ())
	{
		SyncDomain* sync_domain = target.sync_domain ();
		assert (sync_domain || target.is_free_sync_context ());
		call_context_->schedule_call (sync_domain);
	}

	~Synchronized ()
	{
		if (call_context_)
			ScheduleReturn::schedule_return (*call_context_);
	}

	SyncContext& call_context () const
	{
		return *call_context_;
	}

	NIRVANA_NORETURN void on_exception ()
	{
		std::exception_ptr exc = std::current_exception ();
		Core_var <SyncContext> context = std::move (call_context_);
		ScheduleReturn::schedule_return (*context);
		std::rethrow_exception (exc);
	}

private:
	Core_var <SyncContext> call_context_;
};

}
}

#define SYNC_BEGIN(target) { ::Nirvana::Core::Synchronized sync (target); try {
#define SYNC_END() } catch (...) { sync.on_exception (); }}

#endif
