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
#ifndef NIRVANA_CORE_DATASTATE_H_
#define NIRVANA_CORE_DATASTATE_H_
#pragma once

#include <CORBA/Server.h>
#include <Nirvana/System_s.h>
#include "EventSyncTimeout.h"
#include "SyncDomain.h"

namespace Nirvana {
namespace Core {

class DataState :
	public CORBA::servant_traits <Nirvana::DataState>::Servant <DataState>,
	public CORBA::Internal::LifeCycleRefCnt <DataState>,
	public CORBA::Internal::RefCountBase <DataState>,
	public UserObject
{
public:
	using UserObject::operator new;
	using UserObject::operator delete;

	DataState () :
		inconsistent_ (false)
	{
		if (!SyncContext::current ().sync_domain ())
			throw CORBA::BAD_INV_ORDER ();
	}

	void inconststent ()
	{
		if (inconsistent_)
			throw CORBA::TRANSIENT ();
		event_.reset_all ();
		inconsistent_ = true;
	}

	void consistent ()
	{
		if (!inconsistent_)
			throw CORBA::BAD_INV_ORDER ();
		inconsistent_ = false;
		event_.signal_all ();
	}

	bool is_consistent (TimeBase::TimeT timeout)
	{
		if (!inconsistent_)
			return true;
		else if (!timeout)
			return false;
		else
			return event_.wait (timeout);
	}

private:
	EventSyncTimeout event_;
	bool inconsistent_;
};

}
}

#endif
