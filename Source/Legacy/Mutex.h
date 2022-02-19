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
#ifndef NIRVANA_LEGACY_CORE_MUTEX_H_
#define NIRVANA_LEGACY_CORE_MUTEX_H_
#pragma once

#include <CORBA/Server.h>
#include <Nirvana/SimpleList.h>
#include "../IDL/Legacy_s.h"
#include "../LifeCyclePseudo.h"
#include "../MemContextObject.h"
#include "../SyncDomain.h"
#include "../Synchronized.h"

namespace Nirvana {
namespace Legacy {
namespace Core {

class Process;
class ThreadLegacy;

class MutexCore :
	public Nirvana::Core::SyncDomainImpl
{
public:
	MutexCore (Process& parent);

	~MutexCore ();

	void lock ();
	void unlock ();

protected:
	ThreadLegacy* owner_;
	SimpleList <ThreadLegacy> queue_;
};

class MutexUser :
	public MutexCore,
	public CORBA::servant_traits <Nirvana::Legacy::Mutex>::Servant <MutexUser>,
	public Nirvana::Core::LifeCyclePseudo <MutexUser>,
	public Nirvana::Core::MemContextObject
{
public:
	using Nirvana::Core::UserObject::operator new;
	using Nirvana::Core::UserObject::operator delete;

	static Nirvana::Legacy::Mutex::_ref_type create (Process& parent)
	{
		return CORBA::make_pseudo <MutexUser> (std::ref (parent));
	}

	MutexUser (Process& parent) :
		MutexCore (parent)
	{}

	~MutexUser ()
	{}

	bool try_lock ();

	void _add_ref () NIRVANA_NOEXCEPT
	{
		Nirvana::Core::LifeCyclePseudo <MutexUser>::_add_ref ();
	}

	void _remove_ref () NIRVANA_NOEXCEPT
	{
		Nirvana::Core::LifeCyclePseudo <MutexUser>::_remove_ref ();
	}

};

}
}
}

#endif
