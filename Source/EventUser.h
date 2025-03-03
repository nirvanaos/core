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
	public CORBA::servant_traits <Nirvana::Event>::Servant <EventUser>,
	public CORBA::Internal::LifeCycleRefCnt <EventUser>,
	public CORBA::Internal::RefCountBase <EventUser>,
	public UserObject
{
public:
	using UserObject::operator new;
	using UserObject::operator delete;

	EventUser (bool manual_reset, bool initial_state) :
		manual_reset_ (manual_reset)
	{
		if (!SyncContext::current ().sync_domain ())
			throw CORBA::BAD_INV_ORDER ();
		if (initial_state) {
			if (manual_reset)
				event_.signal_one ();
			else
				event_.signal_one ();
		}
	}

	bool wait (const TimeBase::TimeT& timeout)
	{
		bool ret = event_.wait (timeout);
		if (ret && !manual_reset_)
			event_.reset_all ();
		return ret;
	}

	void signal ()
	{
		if (event_.signal_cnt ())
			throw CORBA::BAD_INV_ORDER ();
		if (manual_reset_)
			event_.signal_all ();
		else
			event_.signal_one ();
	}

	void reset ()
	{
		event_.reset_all ();
	}

private:
	EventSyncTimeout event_;
	const bool manual_reset_;
};

}
}

#endif
