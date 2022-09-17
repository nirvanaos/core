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
#include "PortableServer_Current.h"
#include "OtherDomains.h"

using namespace Nirvana::Core;
using namespace Nirvana;

namespace CORBA {

using namespace Internal;

namespace Core {

Nirvana::Core::StaticallyAllocated <Services> Services::singleton_;

Object::_ref_type Services::bind_internal (Service sidx)
{
	if (sidx >= SERVICE_COUNT)
		throw ORB::InvalidName ();

	ServiceRef& ref = services_ [sidx];
	Object::_ref_type ret = ref.get_if_constructed ();
	if (!ret) {
		const Factory& f = factories_ [sidx];
		SYNC_BEGIN (sync_domain_, nullptr);
		if (ref.initialize (f.creation_deadline)) {
			try {
				SYNC_BEGIN (Nirvana::Core::g_core_free_sync_context, new_service_memory ());
				ret = (f.factory) ();
				SYNC_END ();
				ref.finish_construction (ret);
			} catch (...) {
				ref.on_exception ();
				ref.reset ();
				throw;
			}
		} else
			ret = ref.get ();
		SYNC_END ();
	}
	return ret;
}

CoreRef <Service> Services::bind_internal (CoreService sidx)
{
	if (sidx >= CORE_SERVICE_COUNT)
		throw ORB::InvalidName ();

	CoreServiceRef& ref = core_services_ [sidx];
	CoreRef <Nirvana::Core::Service> ret = ref.get_if_constructed ();
	if (!ret) {
		const CoreFactory& f = core_factories_ [sidx];
		SYNC_BEGIN (sync_domain_, nullptr);
		if (ref.initialize (f.creation_deadline)) {
			try {
				SYNC_BEGIN (Nirvana::Core::g_core_free_sync_context, new_service_memory ());
				ret = (f.factory) ();
				SYNC_END ();
				ref.finish_construction (ret);
			} catch (...) {
				ref.on_exception ();
				ref.reset ();
				throw;
			}
		} else
			ret = ref.get ();
		SYNC_END ();
	}
	return ret;
}

// Initial services. Must be lexicographically ordered.

const Services::Factory Services::factories_ [SERVICE_COUNT] = {
	{ "POACurrent", create_POACurrent, 1 * TimeBase::MILLISECOND },
	{ "RootPOA", create_RootPOA, 1 * TimeBase::MILLISECOND }
};

// Service factories

Object::_ref_type Services::create_RootPOA ()
{
	return PortableServer::Core::POA_Root::create ();
}

Object::_ref_type Services::create_POACurrent ()
{
	return make_reference <PortableServer::Core::Current> ()->_this ();
}

// Core services.

const Services::CoreFactory Services::core_factories_ [CORE_SERVICE_COUNT] = {
	{ create_OtherDomains, 1 * TimeBase::MILLISECOND }
};

// Core service factories

CoreRef <Service> Services::create_OtherDomains ()
{
	return CoreRef <Nirvana::Core::Service>::create <ImplDynamic <ESIOP::OtherDomains> > ();
}

}
}
