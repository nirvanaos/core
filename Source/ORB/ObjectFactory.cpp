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
#include "ObjectFactory.h"
#include "Thread.h"
#include <Nirvana/OLF.h>

namespace CORBA {
namespace Internal {
namespace Core {

using namespace ::Nirvana;
using namespace ::Nirvana::Core;

void ObjectFactory::stateless_creation_frame (CORBA::Internal::ObjectFactory::
	StatelessCreationFrame* scf)
{
	MemContext::current ().get_TLS ().set (TLS::CORE_TLS_OBJECT_FACTORY, scf);
}

CORBA::Internal::ObjectFactory::StatelessCreationFrame* ObjectFactory::
stateless_creation_frame ()
{
	return reinterpret_cast <CORBA::Internal::ObjectFactory::StatelessCreationFrame*>
		(MemContext::current ().get_TLS ().get (TLS::CORE_TLS_OBJECT_FACTORY));
}

void ObjectFactory::check_context ()
{
	if (ExecDomain::RestrictedMode::NO_RESTRICTIONS != 
		Nirvana::Core::ExecDomain::current ().restricted_mode ())
		throw_NO_PERMISSION ();

	CORBA::Internal::ObjectFactory::StatelessCreationFrame* scs =
		stateless_creation_frame ();
	if (!scs) {
		// Stateful object must be created in the sync doimain only.
		ExecDomain* ed = ::Nirvana::Core::Thread::current ().exec_domain ();
		if (!ed || !ed->sync_context ().sync_domain ())
			throw_BAD_INV_ORDER ();
	}
}

}

extern const ::Nirvana::ImportInterfaceT <ObjectFactory> g_object_factory = {
	::Nirvana::OLF_IMPORT_INTERFACE, "CORBA/Internal/g_object_factory",
	ObjectFactory::repository_id_, NIRVANA_STATIC_BRIDGE (ObjectFactory,
	Core::ObjectFactory)
};

}
}

NIRVANA_EXPORT (_exp_CORBA_Internal_g_object_factory, "CORBA/Internal/g_object_factory", CORBA::Internal::ObjectFactory, CORBA::Internal::Core::ObjectFactory)
