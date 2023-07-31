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

namespace CORBA {
namespace Core {

class PollableSet;

class Pollable :
	public Internal::ValueImpl <Pollable, CORBA::ValueBase>,
	public Internal::ValueTraits <Pollable>,
	public Internal::ValueNonTruncatable,
	public Internal::ValueBaseNoFactory,
	public Internal::ValueImpl <Pollable, CORBA::Pollable>,
	public servant_traits <CORBA::Internal::RequestCallback>
{
public:
	Interface* _query_valuetype (Internal::String_in id) noexcept
	{
		if (id.empty () || Internal::RepId::compatible (Internal::RepIdOf <CORBA::Pollable>::id, id))
			return &static_cast <Bridge <CORBA::Pollable>&> (*this);
		return nullptr;
	}

	static Pollable* cast (CORBA::Pollable::_ptr_type ptr) noexcept
	{
		return static_cast <Pollable*> (
			static_cast <Internal::Bridge <CORBA::Pollable>*> (&ptr));
	}

	bool is_ready (ULong timeout)
	{
		return event_.wait (timeout * TimeBase::MILLISECOND);
	}

	static CORBA::PollableSet::_ref_type create_pollable_set ()
	{
		return make_reference <PollableSet> ()->_this ();
	}

	PollableSet* cur_set () const noexcept
	{
		return cur_set_;
	}

	void cur_set (PollableSet* ps) noexcept
	{
		assert (!cur_set_);
		cur_set_ = ps;
	}

	void completed (Internal::IORequest::_ptr_type rq)
	{
		event_.signal_all ();
		if (cur_set_)
			cur_set_->pollable_ready ();
	}

protected:
	Pollable () :
		cur_set_ (nullptr)
	{}

private:
	Nirvana::Core::EventSync event_;
	PollableSet* cur_set_;
};

class DIIPollable :
	public Pollable,
	public Internal::ValueImpl <DIIPollable, CORBA::DIIPollable>
{
public:
	Interface* _query_valuetype (Internal::String_in id) noexcept
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

	void add_pollable (Pollable::_ptr_type potential)
	{
		Pollable* p = Pollable::cast (potential);
		if (!p)
			throw BAD_PARAM ();
		if (p->cur_set ())
			throw BAD_PARAM (MAKE_OMG_MINOR (43));
		set_.insert (p);
		p->cur_set (this);
	}

	CORBA::Pollable::_ref_type get_ready_pollable (uint32_t timeout)
	{
		if (event_.wait (timeout)) {
			for (Set::iterator it = set_.begin (); it != set_.end (); ++it) {
				Pollable* p = *it;
				if (p->is_ready (0)) {
					CORBA::Pollable::_ref_type ret (p);
					set_.erase (it);
					return ret;
				}
			}
		}
		return nullptr;
	}

	void remove (Pollable::_ptr_type potential)
	{
		Pollable* p = Pollable::cast (potential);
		if (!p || p->cur_set () != this)
			throw UnknownPollable ();
		if (p->is_ready (0))
			event_.reset_one ();
		set_.erase (p);
	}

	uint16_t number_left () const
	{
		return (uint16_t)std::max (set_.size (), (size_t)std::numeric_limits <uint16_t>::max ());
	}

	void pollable_ready ()
	{
		event_.signal_one ();
	}

private:
	typedef Nirvana::Core::SetUnorderedUnstable <servant_reference <Pollable>,
		std::hash <void*>, std::equal_to <void*>, Nirvana::Core::UserAllocator> Set;

	Set set_;
	Nirvana::Core::EventSync event_;
};

}
}

#endif
