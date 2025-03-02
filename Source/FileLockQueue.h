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
#ifndef NIRVANA_CORE_FILELOCKQUEUE_H_
#define NIRVANA_CORE_FILELOCKQUEUE_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include <Nirvana/File.h>
#include <Nirvana/SimpleList.h>
#include "UserObject.h"
#include "EventSync.h"
#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

class FileLockQueue
{
public:
	~FileLockQueue ()
	{
		CORBA::OBJECT_NOT_EXIST exc;
		while (!list_.empty ()) {
			dequeue (list_.begin (), exc);
		}
	}

	class Entry :
		public SimpleList <Entry>::Element,
		public UserObject
	{
	public:
		Entry (const FileSize& begin, const FileSize& end, LockType level_max, LockType level_min,
			const void* owner, const TimeBase::TimeT& timeout) noexcept :
			begin_ (begin),
			end_ (end),
			deadline_ (ExecDomain::current ().deadline ()),
			expire_time_ (calc_expire_time (timeout)),
			owner_ (owner),
			level_max_ (level_max),
			level_min_ (level_min)
		{}

		const FileSize& begin () const noexcept
		{
			return begin_;
		}

		const FileSize& end () const noexcept
		{
			return end_;
		}

		LockType level_max () const noexcept
		{
			return level_max_;
		}

		LockType level_min () const noexcept
		{
			return level_min_;
		}

		const void* owner () const noexcept
		{
			return owner_;
		}

		const DeadlineTime& deadline () const noexcept
		{
			return deadline_;
		}

		LockType wait ()
		{
			try {
				event_.wait ();
			} catch (...) {
				delete this;
				throw;
			}
			LockType ret = level_max_;
			delete this;
			return ret;
		}

		void signal (LockType level) noexcept
		{
			level_max_ = level;
			event_.signal ();
		}

		void signal (const CORBA::Exception& exc) noexcept
		{
			event_.signal (exc);
		}

		const SteadyTime& expire_time () const noexcept
		{
			return expire_time_;
		}

	private:
		static SteadyTime calc_expire_time (const TimeBase::TimeT& timeout) noexcept
		{
			assert (timeout);
			SteadyTime exp;
			if (timeout < std::numeric_limits <TimeBase::TimeT>::max ()) {
				SteadyTime time = Chrono::steady_clock ();
				exp = time + timeout;
				if (exp < time) // Overflow
					throw_BAD_PARAM ();
			} else
				exp = std::numeric_limits <SteadyTime>::max ();
			return exp;
		}

	private:
		FileSize begin_;
		FileSize end_;
		DeadlineTime deadline_;
		SteadyTime expire_time_;
		const void* owner_;
		EventSync event_;
		LockType level_max_;
		LockType level_min_;
	};

	typedef SimpleList <Entry>::iterator iterator;

	LockType enqueue (const FileSize& begin, const FileSize& end,
		LockType level_max, LockType level_min,
		const void* owner, const TimeBase::TimeT& timeout)
	{
		Entry* entry = new Entry (begin, end, level_max, level_min, owner, timeout);
		iterator ins = list_.end ();
		while (ins != list_.begin ()) {
			iterator prev = ins;
			--prev;
			if (prev->deadline () > entry->deadline ())
				ins = prev;
			else
				break;
		}
		entry->insert (*ins);
		return entry->wait ();
	}
	
	iterator begin () const noexcept
	{
		return list_.begin ();
	}

	iterator end () const noexcept
	{
		return list_.end ();
	}

	iterator dequeue (iterator it, LockType level) noexcept
	{
		iterator next = it->next ();
		list_.remove (it);
		it->signal (level);
		return next;
	}

	iterator dequeue (iterator it, const CORBA::Exception& exc) noexcept
	{
		iterator next = it->next ();
		list_.remove (it);
		it->signal (exc);
		return next;
	}

	bool empty () const noexcept
	{
		return list_.empty ();
	}

	void on_delete_proxy (const void* owner) noexcept
	{
		for (iterator it = begin (); it != end ();) {
			if (it->owner () == owner)
				it = dequeue (it, LockType::LOCK_NONE);
			else
				++it;
		}
	}

	void housekeeping () noexcept
	{
		SteadyTime time = Chrono::steady_clock ();
		for (iterator it = begin (); it != end ();) {
			if (it->expire_time () <= time)
				it = dequeue (it, LockType::LOCK_NONE);
			else
				++it;
		}
	}

private:
	SimpleList <Entry> list_;
};

}
}

#endif
