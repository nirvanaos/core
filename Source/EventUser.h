/// \file
/*
* Nirvana runtime library.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2025 Igor Popov.
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
#ifndef NIRVANA_CORE_EVENTUSER_H_
#define NIRVANA_CORE_EVENTUSER_H_
#pragma once

#include <CORBA/Server.h>
#include <Nirvana/System_s.h>
#include "EventSyncTimeout.h"
#include "SyncDomain.h"

namespace Nirvana {
namespace Core {

class EventUser :
	public IDL::traits <Nirvana::Event>::Servant <EventUser>,
	private EventSyncTimeout,
	public UserObject
{
public:
	using UserObject::operator new;
	using UserObject::operator delete;

	EventUser (bool manual_reset, bool initial_state) :
		EventSyncTimeout (initial_state ?
			(manual_reset ? SIGNAL_ALL : 1) : 0),
		manual_reset_ (manual_reset)
#ifndef NDEBUG
		,dbg_sc_ (SyncContext::current ())
#endif
	{
		if (!SyncContext::current ().sync_domain ())
			throw CORBA::BAD_INV_ORDER ();
	}

	bool wait (const TimeBase::TimeT& timeout)
	{
		assert (&dbg_sc_ == &SyncContext::current ());

		bool ret = EventSyncTimeout::wait (timeout);
		if (ret && !manual_reset_)
			EventSyncTimeout::reset_all ();
		return ret;
	}

	void signal ()
	{
		assert (&dbg_sc_ == &SyncContext::current ());

		if (EventSyncTimeout::signal_cnt ())
			throw CORBA::BAD_INV_ORDER ();
		if (manual_reset_)
			EventSyncTimeout::signal_all ();
		else
			EventSyncTimeout::signal_one ();
	}

	void reset ()
	{
		assert (&dbg_sc_ == &SyncContext::current ());
		EventSyncTimeout::reset_all ();
	}

private:
	const bool manual_reset_;

#ifndef NDEBUG
	const SyncContext& dbg_sc_;
#endif
};

}
}

#endif
