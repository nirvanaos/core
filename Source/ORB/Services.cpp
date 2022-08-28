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

#include "Services.h"
#include "POA_Root.h"

using namespace Nirvana::Core;
using namespace Nirvana;

namespace CORBA {
namespace Core {

Nirvana::Core::StaticallyAllocated <Services> Services::singleton_;

Object::_ref_type Services::bind (Service sidx)
{
	if ((size_t)sidx >= Service::SERVICE_COUNT)
		throw ORB::InvalidName ();

	Object::_ref_type ret = singleton_->services_ [sidx].get_if_constructed ();
	if (!ret) {
		SYNC_BEGIN (singleton_->sync_domain_, nullptr);
		ret = singleton_->bind_service_sync (sidx);
		SYNC_END ();
	}
	return ret;
}

Object::_ref_type Services::bind_service_sync (Service sidx)
{
	assert ((size_t)sidx < Service::SERVICE_COUNT);
	ServiceRef& ref = services_ [sidx];
	const Factory& f = factories_ [sidx];
	Object::_ref_type svc;
	if (ref.initialize (f.creation_deadline)) {
		try {
			SYNC_BEGIN (Nirvana::Core::g_core_free_sync_context, new_service_memory ());
			svc = (f.factory) ();
			SYNC_END ();
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

// Initial services. Must be lexicographically ordered.

const Services::Factory Services::factories_ [Service::SERVICE_COUNT] = {
	{ "RootPOA", create_RootPOA, 1 * TimeBase::MILLISECOND }
};

// Service factories

Object::_ref_type Services::create_RootPOA ()
{
	return PortableServer::Core::POA_Root::create ();
}

}
}
