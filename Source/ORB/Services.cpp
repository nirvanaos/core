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
#include "../pch.h"
#include "Services.h"
#include "Scheduler.h"
#include "POA_Root.h"
#include "PortableServer_Current.h"
#include "NameService/NameService.h"
#include "ESIOP.h"
#include "../Console.h"

namespace Nirvana {
namespace Core {

extern CORBA::Object::_ref_type create_SysDomain ();
extern CORBA::Object::_ref_type create_ProtDomain ();
extern CORBA::Object::_ref_type create_ProtDomain ();

}
}

namespace CosNaming {
namespace Core {

extern CORBA::Object::_ref_type create_NameService ();

}
}

using namespace Nirvana::Core;
using namespace Nirvana;

namespace CORBA {

using namespace Internal;

namespace Core {

extern Object::_ref_type create_TC_Factory ();

Nirvana::Core::StaticallyAllocated <Services> Services::singleton_;

Services::~Services ()
{
	if (ESIOP::is_system_domain ()) {
		Object::_ref_type name_service = services_ [NameService].get_if_constructed ();
		if (name_service)
			CosNaming::Core::NameService::shutdown (name_service);
	}

	Object::_ref_type POA_root = services_ [RootPOA].get_if_constructed ();
	if (POA_root) // Block incoming requests and complete currently executed ones.
		PortableServer::Core::POA_Root::shutdown (POA_root);
}

inline Object::_ref_type Services::bind_internal (Service sidx)
{
	if (sidx >= SERVICE_COUNT)
		throw ORB::InvalidName ();

	ServiceRef& ref = services_ [sidx];
	Object::_ref_type ret = ref.get_if_constructed ();
	if (!ret) {
		if (Scheduler::state () != Scheduler::RUNNING)
			throw INITIALIZE ();

		// Create service
		const Factory& f = factories_ [sidx];
		SYNC_BEGIN (sync_domain_, nullptr);
		if (ref.initialize (f.creation_deadline)) {
			auto wait_list = ref.wait_list ();
			try {
				SYNC_BEGIN (Nirvana::Core::g_core_free_sync_context, new_service_memory ());
				ret = (f.factory) ();
				SYNC_END ();
				wait_list->finish_construction (ret);
			} catch (...) {
				wait_list->on_exception ();
				ref.reset ();
				throw;
			}
		} else
			ret = ref.get ();
		SYNC_END ();
	}
	return ret;
}

Object::_ref_type Services::bind (Service sidx)
{
	if (Nirvana::Core::Scheduler::state () != Nirvana::Core::Scheduler::RUNNING)
		return nullptr;
	return singleton_->bind_internal (sidx);
}

// Initial services. Must be lexicographically ordered.

const Services::UserService Services::user_services_ [USER_SERVICE_COUNT] = {
	{ "Console" },
	{ "NameService" },
	{ "POACurrent" },
	{ "ProtDomain" },
	{ "RootPOA" },
	{ "SysDomain" }
};

const Services::Factory Services::factories_ [SERVICE_COUNT] = {
	{ create_Console, 1 * TimeBase::MILLISECOND },
	{ CosNaming::Core::create_NameService, 1 * TimeBase::MILLISECOND },
	{ create_POACurrent, 1 * TimeBase::MILLISECOND },
	{ create_ProtDomain, 1 * TimeBase::MILLISECOND },
	{ PortableServer::Core::POA_Root::create, 1 * TimeBase::MILLISECOND },
	{ create_SysDomain, 10 * TimeBase::MILLISECOND }, // May cause inter-domain call

	// Internal services are below

	{ create_TC_Factory, 1 * TimeBase::MILLISECOND }
};

// Service factories

Object::_ref_type Services::create_POACurrent ()
{
	return make_reference <PortableServer::Core::Current> ()->_this ();
}

Object::_ref_type Services::create_Console ()
{
	return make_reference <Nirvana::Core::Console> ()->_this ();
}

}
}
