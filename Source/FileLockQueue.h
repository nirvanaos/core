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
#include "ExecDomain.h"
#include "TimerAsyncCall.h"
#include "Chrono.h"

namespace Nirvana {
namespace Core {

class FileLockRanges;

class FileLockQueue
{
	static const TimeBase::TimeT TIMER_DEADLINE = 1 * TimeBase::MILLISECOND;

public:
	FileLockQueue () :
		list_ (nullptr),
		nearest_expire_time_ (std::numeric_limits <TimeBase::TimeT>::max ())
	{}

	~FileLockQueue ()
	{
		if (timer_)
			timer_->disconnect ();
		signal_all (CORBA::OBJECT_NOT_EXIST ());
	}

	class Entry
	{
	public:
		Entry (const FileSize& begin, const FileSize& end, LockType level_max, LockType level_min,
			const void* owner, const TimeBase::TimeT& timeout) noexcept :
			begin_ (begin),
			end_ (end),
			expire_time_ (calc_expire_time (timeout)),
			exec_domain_ (ExecDomain::current ()),
			owner_ (owner),
			next_ (nullptr),
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
			return exec_domain_.deadline ();
		}

		ExecDomain& exec_domain () const noexcept
		{
			return exec_domain_;
		}

		void signal (LockType level) noexcept
		{
			level_max_ = level;
			exec_domain_.resume ();
		}

		void signal (const CORBA::Exception& exc) noexcept
		{
			exec_domain_.resume (exc);
		}

		const SteadyTime& expire_time () const noexcept
		{
			return expire_time_;
		}

		Entry* next () const noexcept
		{
			return next_;
		}

		void next (Entry* p) noexcept
		{
			next_ = p;
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
		SteadyTime expire_time_;
		ExecDomain& exec_domain_;
		const void* owner_;
		Entry* next_;
		LockType level_max_;
		LockType level_min_;
	};

	LockType enqueue (const FileSize& begin, const FileSize& end,
		LockType level_max, LockType level_min,
		const void* owner, const TimeBase::TimeT& timeout)
	{
		Entry entry (begin, end, level_max, level_min, owner, timeout);
		DeadlineTime deadline = entry.deadline ();

		Entry* prev = nullptr;
		for (Entry* next = list_; next; next = next->next ()) {
			if (next->deadline () > deadline)
				break;
			prev = next;
		}

		entry.exec_domain ().suspend_prepare ();

		if (prev) {
			entry.next (prev->next ());
			prev->next (&entry);
		} else
			list_ = &entry;

		if (nearest_expire_time_ > entry.expire_time ()) {
			nearest_expire_time_ = entry.expire_time ();
			if (!timer_)
				timer_ = Ref <Timer>::create <ImplDynamic <Timer> > (std::ref (*this));
			timer_->set (0, entry.expire_time () - Chrono::steady_clock (), 0);
		}

		entry.exec_domain ().suspend ();

		return entry.level_max ();
	}
	
	Entry* dequeue (Entry* prev, Entry* entry) noexcept;

	bool empty () const noexcept
	{
		return !list_;
	}

	void on_delete_proxy (const void* owner) noexcept
	{
		bool changed = false;
		for (Entry* entry = list_, *prev = nullptr; entry;) {
			if (entry->owner () == owner) {
				Entry* next = dequeue (prev, entry);
				entry->signal (LockType::LOCK_NONE);
				entry = next;
				changed = true;
			} else {
				prev = entry;
				entry = entry->next ();
			}
		}
		if (changed)
			adjust_timer ();
	}

	void retry_lock (FileLockRanges& lock_ranges) noexcept;

private:
	void on_timer () noexcept
	{
		if (Scheduler::shutdown_started ()) {
			if (timer_)
				timer_->cancel ();
			while (list_) {
				Entry* entry = list_;
				list_ = entry->next ();
				if (entry->expire_time () < std::numeric_limits <SteadyTime>::max ())
					entry->signal (LockType::LOCK_NONE);
				else
					entry->signal (CORBA::BAD_INV_ORDER ());
			}
		} else {
			SteadyTime time = Chrono::steady_clock ();
			TimeBase::TimeT next_time = std::numeric_limits <TimeBase::TimeT>::max ();
			for (Entry* entry = list_, *prev = nullptr; entry;) {
				if (entry->expire_time () <= time) {
					Entry* next = dequeue (prev, entry);
					entry->signal (LockType::LOCK_NONE);
					entry = next;
				} else {
					if (next_time > entry->expire_time ())
						next_time = entry->expire_time ();
					prev = entry;
					entry = entry->next ();
				}
			}
			nearest_expire_time (next_time);
		}
	}

	void adjust_timer () noexcept;
	void nearest_expire_time (TimeBase::TimeT t) noexcept;
	void signal_all (const CORBA::Exception& exc) noexcept;

	class Timer : public TimerAsyncCall
	{
	public:
		void disconnect () noexcept
		{
			obj_ = nullptr;
			TimerAsyncCall::cancel ();
		}

	protected:
		Timer (FileLockQueue& obj) :
			TimerAsyncCall (SyncContext::current (), TIMER_DEADLINE),
			obj_ (&obj)
		{}

	private:
		void run (const TimeBase::TimeT& signal_time) override;

	private:
		FileLockQueue* obj_;
	};

private:
	Entry* list_;
	Ref <Timer> timer_;
	TimeBase::TimeT nearest_expire_time_;
};

}
}

#endif
