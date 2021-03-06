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
#include <algorithm>

namespace CORBA {
namespace Internal {
namespace Core {

class Services
{
public:
	enum Service
	{
		DefaultPOA,

		SERVICE_COUNT
	};

protected:
	static Service find_service (const char* id) NIRVANA_NOEXCEPT
	{
		const Factory* f = std::lower_bound (factories_, std::end (factories_), id, Less ());
		if (f < std::end (factories_) && strcmp (id, f->id))
			f = std::end (factories_);
		return (Service)(f - factories_);
	}

	Object::_ref_type bind_service_sync (Service sidx)
	{
		assert ((size_t)sidx < Service::SERVICE_COUNT);
		ServiceRef& ref = services_ [sidx];
		const Factory& f = factories_ [sidx];
		Object::_ref_type svc;
		if (ref.initialize (f.creation_deadline)) {
			try {
				svc = (f.factory) ();
				ref.finish_construction (svc);
			} catch (...) {
				ref.on_exception ();
				ref.reset ();
				throw;
			}
		} else
			svc = ref.get ();
		return svc;
	}

	void reset () NIRVANA_NOEXCEPT
	{
		for (ServiceRef* p = services_; p != std::end (services_); ++p) {
			p->reset ();
		}
	}

private:
	// Service factories

	static Object::_ref_type create_DefaultPOA ();

private:
	struct Factory
	{
		const char* id;
		Object::_ref_type (*factory) ();
		Nirvana::DeadlineTime creation_deadline;
	};

	struct Less
	{
		bool operator () (const Factory& l, const char* r) const NIRVANA_NOEXCEPT
		{
			return strcmp (l.id, r) < 0;
		}

		bool operator () (const char* l, const Factory& r) const NIRVANA_NOEXCEPT
		{
			return strcmp (l, r.id) < 0;
		}
	};

	typedef Nirvana::Core::WaitableRef <Object::_ref_type> ServiceRef;

private:
	ServiceRef services_ [Service::SERVICE_COUNT];

	// Services
	static const Factory factories_ [Service::SERVICE_COUNT];
};

}
}
}

#endif
