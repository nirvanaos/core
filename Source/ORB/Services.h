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
#include "../SyncDomain.h"
#include <algorithm>

namespace CORBA {
namespace Core {

class Services
{
public:
	enum Service
	{
		Console,
		NameService,
		POACurrent,
		ProtDomain,
		RootPOA,
		SysDomain,

		// End of user available services.
		// Services below are internal.

		TC_Factory,

		SERVICE_COUNT
	};

	static const size_t USER_SERVICE_COUNT = TC_Factory;

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
		for (const UserService* s = user_services_; s != std::end (user_services_); ++s)
			list.emplace_back (s->id);
		return list;
	}

	Services () :
		sync_domain_ (std::ref (Nirvana::Core::Heap::shared_heap ()))
	{}

	~Services ();

	static void initialize () noexcept
	{
		singleton_.construct ();
	}

	static void terminate () noexcept
	{
		singleton_.destruct ();
	}

private:
	static Service find_service (const Char* id) noexcept
	{
		const UserService* f = std::lower_bound (user_services_, std::end (user_services_), id, Less ());
		if (f < std::end (user_services_) && strcmp (id, f->id))
			f = std::end (user_services_);
		return (Service)(f - user_services_);
	}

	Object::_ref_type bind_internal (Service sidx);

private:
	static Nirvana::Core::Heap* new_service_memory () noexcept
	{
		return sizeof (void*) >= 4 ?
			nullptr // Create new memory context
			:
			&Nirvana::Core::Heap::shared_heap ();
	}

	// Service factories
	static Object::_ref_type create_POACurrent ();
	static Object::_ref_type create_Console ();

private:
	struct UserService
	{
		const Char* id;
	};

	struct Less
	{
		bool operator () (const UserService& l, const Char* r) const noexcept
		{
			return strcmp (l.id, r) < 0;
		}

		bool operator () (const Char* l, const UserService& r) const noexcept
		{
			return strcmp (l, r.id) < 0;
		}
	};

	struct Factory
	{
		Object::_ref_type (*factory) ();
		TimeBase::TimeT creation_deadline;
	};

	typedef Nirvana::Core::WaitableRef <Object::_ref_type> ServiceRef;

	Nirvana::Core::ImplStatic <Nirvana::Core::SyncDomainCore> sync_domain_;
	ServiceRef services_ [SERVICE_COUNT];

	static Nirvana::Core::StaticallyAllocated <Services> singleton_;

	static const UserService user_services_ [USER_SERVICE_COUNT];
	static const Factory factories_ [SERVICE_COUNT];
};

}
}

#endif
