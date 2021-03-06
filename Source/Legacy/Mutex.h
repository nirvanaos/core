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

#include "Process.h"
#include "../IDL/Legacy_s.h"
#include "../LifeCyclePseudo.h"
#include "../Synchronized.h"

namespace Nirvana {
namespace Legacy {
namespace Core {

class Process;
class ThreadBase;

class Mutex :
	public CORBA::servant_traits <Nirvana::Legacy::Mutex>::Servant <Mutex>,
	public Nirvana::Core::LifeCyclePseudo <Mutex>,
	public Nirvana::Core::SyncDomainImpl,
	public Nirvana::Core::MemContextObject
{
public:
	using Nirvana::Core::UserObject::operator new;
	using Nirvana::Core::UserObject::operator delete;

	static Nirvana::Legacy::Mutex::_ref_type create (Process& parent)
	{
		return CORBA::make_pseudo <Mutex> (std::ref (parent));
	}

	Mutex (Process& parent) :
		Nirvana::Core::SyncDomainImpl (parent.sync_context (), parent),
		owner_ (nullptr)
	{}

	~Mutex ()
	{
		// TODO: Terminate all waiting threads?
	}

	void lock ()
	{
		ThreadBase& thread = ThreadBase::current ();
		SYNC_BEGIN (*this, nullptr);
		if (!owner_) {
			owner_ = &thread;
			return;
		} else if (owner_ == &thread)
			throw_BAD_INV_ORDER ();
		else {
			queue_.push_back (thread);
			_sync_frame.suspend_and_return ();
		}
		SYNC_END ();
	}

	void unlock ()
	{
		ThreadBase& thread = ThreadBase::current ();
		SYNC_BEGIN (*this, nullptr);
		if (owner_ != &thread)
			throw_BAD_INV_ORDER ();
		owner_ = nullptr;
		if (!queue_.empty ()) {
			ThreadBase& next = queue_.front ();
			next.remove (); // From queue
			owner_ = &next;
			next.exec_domain ()->resume ();
		}
		SYNC_END ();
	}

	bool try_lock ()
	{
		SYNC_BEGIN (*this, nullptr);
		if (!owner_) {
			owner_ = &ThreadBase::current ();
			return true;
		}
		SYNC_END ();
		return false;
	}

	void _add_ref () NIRVANA_NOEXCEPT
	{
		Nirvana::Core::LifeCyclePseudo <Mutex>::_add_ref ();
	}

	void _remove_ref () NIRVANA_NOEXCEPT
	{
		Nirvana::Core::LifeCyclePseudo <Mutex>::_remove_ref ();
	}

private:
	ThreadBase* owner_;
	SimpleList <ThreadBase> queue_;
};

}
}
}

#endif
