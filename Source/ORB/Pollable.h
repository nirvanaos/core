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
#ifndef NIRVANA_ORB_CORE_POLLABLE_H_
#define NIRVANA_ORB_CORE_POLLABLE_H_
#pragma once

#include <CORBA/Server.h>
#include <CORBA/Pollable_s.h>
#include <CORBA/Proxy/IOReference_s.h>
#include "../EventSync.h"
#include "../MapUnorderedUnstable.h"
#include "../Synchronized.h"

namespace CORBA {
namespace Core {

class PollableSet;

class NIRVANA_NOVTABLE Pollable :
	public servant_traits <CORBA::Pollable>::Servant <Pollable>
{
	typedef servant_traits <CORBA::Pollable>::Servant <Pollable> Servant;

public:
	static Pollable* cast (CORBA::Pollable::_ptr_type ptr) noexcept
	{
		return static_cast <Pollable*> (
			static_cast <Internal::Bridge <CORBA::Pollable>*> (&ptr));
	}

	virtual Internal::Interface* _query_valuetype (const IDL::String& id) noexcept
	{
		return Servant::_query_valuetype (id);
	}

	// Outline, called also in Poller
	bool is_ready (ULong timeout);

	CORBA::PollableSet::_ref_type create_pollable_set () const;

	void cur_set (PollableSet* ps) noexcept
	{
		SYNC_BEGIN (*sync_domain_, nullptr)
		if (cur_set_ && ps)
			throw BAD_PARAM (MAKE_OMG_MINOR (43));
		cur_set_ = ps;
		SYNC_END ()
	}

	void completed (Internal::IORequest::_ptr_type rq)
	{
		SYNC_BEGIN (*sync_domain_, nullptr)
			on_complete (rq);
		SYNC_END ()
	}

	Pollable (const Pollable& src);
	~Pollable ();

protected:
	Pollable ();

	virtual void on_complete (Internal::IORequest::_ptr_type reply);

	Nirvana::Core::SyncDomain& sync_domain () noexcept
	{
		return *sync_domain_;
	}

private:
	servant_reference <Nirvana::Core::SyncDomain> sync_domain_;
	Nirvana::Core::EventSync event_;
	PollableSet* cur_set_;
	bool ready_;
};

class DIIPollable :
	public Pollable,
	public Internal::ServantTraits <DIIPollable>,
	public Internal::ValueImpl <DIIPollable, CORBA::DIIPollable>,
	public CORBA::Internal::LifeCycleRefCnt <DIIPollable>
{
public:
	using CORBA::Internal::ServantTraits <DIIPollable>::_wide_val;
	using CORBA::Internal::ServantTraits <DIIPollable>::_implementation;
	using CORBA::Internal::LifeCycleRefCnt <DIIPollable>::__duplicate;
	using CORBA::Internal::LifeCycleRefCnt <DIIPollable>::__release;
	using CORBA::Internal::LifeCycleRefCnt <DIIPollable>::_duplicate;
	using CORBA::Internal::LifeCycleRefCnt <DIIPollable>::_release;

	virtual Internal::Interface* _query_valuetype (const IDL::String& id) noexcept override
	{
		return Internal::FindInterface <CORBA::DIIPollable, CORBA::Pollable>::find (*this, id);
	}
};

class PollableSet : public servant_traits <CORBA::PollableSet>::Servant <PollableSet>
{
public:
	static CORBA::DIIPollable::_ref_type create_dii_pollable ()
	{
		return make_reference <DIIPollable> ();
	}

	void add_pollable (CORBA::Pollable::_ptr_type potential)
	{
		servant_reference <Pollable> p = Pollable::cast (potential);
		if (!p)
			throw BAD_PARAM ();
		p->cur_set (this);
		try {
			set_.insert (p);
		} catch (...) {
			p->cur_set (nullptr);
			throw;
		}
	}

	CORBA::Pollable::_ref_type get_ready_pollable (uint32_t timeout)
	{
		if (event_.wait (Nirvana::Core::EventSync::ms2time (timeout))) {
			for (Set::iterator it = set_.begin (); it != set_.end (); ++it) {
				Pollable* p = *it;
				if (p->is_ready (0)) { // Does not cause context switch
					CORBA::Pollable::_ref_type ret (p);
					set_.erase (it);
					return ret;
				}
			}
		}
		return nullptr;
	}

	void remove (CORBA::Pollable::_ptr_type potential)
	{
		servant_reference <Pollable> p = Pollable::cast (potential);
		if (!p || !set_.erase (p))
			throw UnknownPollable ();
		if (p->is_ready (0))
			event_.reset_one ();
		p->cur_set (nullptr);
	}

	uint16_t number_left () const
	{
		return (uint16_t)std::max (set_.size (), (size_t)std::numeric_limits <uint16_t>::max ());
	}

	void pollable_ready ();

private:
	typedef Nirvana::Core::SetUnorderedUnstable <servant_reference <Pollable>,
		std::hash <void*>, std::equal_to <void*>, Nirvana::Core::UserAllocator> Set;

	Set set_;
	Nirvana::Core::EventSync event_;
};

inline
CORBA::PollableSet::_ref_type Pollable::create_pollable_set () const
{
	SYNC_BEGIN (*sync_domain_, nullptr)
		return make_reference <PollableSet> ()->_this ();
	SYNC_END ()
}

}
}

#endif