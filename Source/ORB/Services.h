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
#ifndef NIRVANA_ORB_CORE_SERVICES_H_
#define NIRVANA_ORB_CORE_SERVICES_H_
#pragma once

#include <CORBA/CORBA.h>
#include "../WaitableRef.h"
#include "../Synchronized.h"
#include <algorithm>

namespace CORBA {
namespace Core {

class Services
{
public:
	enum Service
	{
		POACurrent,
		RootPOA,

		SERVICE_COUNT
	};

	/// Bind service.
	/// 
	/// \param sidx Service index.
	/// \returns Service object reference.
	static Object::_ref_type bind (Service sidx);

	/// Bind service.
	/// 
	/// \param id Service identifier.
	/// \returns Service object reference.
	static Object::_ref_type bind (const IDL::String& id)
	{
		return bind (find_service (id.c_str ()));
	}

	static ORB::ObjectIdList list_initial_services ()
	{
		ORB::ObjectIdList list;
		list.reserve (SERVICE_COUNT);
		for (const Factory* f = factories_; f != std::end (factories_); ++f)
			list.emplace_back (f->id);
		return list;
	}

	Services () :
		sync_domain_ (std::ref (static_cast <Nirvana::Core::MemContextCore&> (
			Nirvana::Core::g_shared_mem_context)))
	{}

	~Services ()
	{}

	static void initialize () NIRVANA_NOEXCEPT
	{
		singleton_.construct ();
	}

	static void terminate () NIRVANA_NOEXCEPT
	{
		singleton_.destruct ();
	}

protected:
	static Service find_service (const Char* id) NIRVANA_NOEXCEPT
	{
		const Factory* f = std::lower_bound (factories_, std::end (factories_), id, Less ());
		if (f < std::end (factories_) && strcmp (id, f->id))
			f = std::end (factories_);
		return (Service)(f - factories_);
	}

	Object::_ref_type bind_service_sync (Service sidx);

private:
	static Nirvana::Core::MemContext* new_service_memory () NIRVANA_NOEXCEPT
	{
		return sizeof (void*) > 4 ?
			nullptr // Create new memory context
			:
			&Nirvana::Core::g_shared_mem_context;
	}

	// Service factories
	static Object::_ref_type create_RootPOA ();
	static Object::_ref_type create_POACurrent ();

private:
	struct Factory
	{
		const Char* id;
		Object::_ref_type (*factory) ();
		TimeBase::TimeT creation_deadline;
	};

	struct Less
	{
		bool operator () (const Factory& l, const Char* r) const NIRVANA_NOEXCEPT
		{
			return strcmp (l.id, r) < 0;
		}

		bool operator () (const Char* l, const Factory& r) const NIRVANA_NOEXCEPT
		{
			return strcmp (l, r.id) < 0;
		}
	};

	typedef Nirvana::Core::WaitableRef <Object::_ref_type> ServiceRef;

private:
	Nirvana::Core::ImplStatic <Nirvana::Core::SyncDomainCore> sync_domain_;
	ServiceRef services_ [Service::SERVICE_COUNT];

	static Nirvana::Core::StaticallyAllocated <Services> singleton_;

	// Services
	static const Factory factories_ [Service::SERVICE_COUNT];
};

}
}

#endif
