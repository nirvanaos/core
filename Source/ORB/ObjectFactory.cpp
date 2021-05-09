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
#include "Thread.inl"
#include <Nirvana/OLF.h>

namespace CORBA {
namespace Nirvana {

namespace Core {

using namespace ::Nirvana;
using namespace ::Nirvana::Core;

CORBA::Nirvana::ObjectFactory::StatelessCreationFrame* ObjectFactory::stateless_creation_frame ()
{
	ExecDomain* ed = ::Nirvana::Core::Thread::current ().exec_domain ();
	assert (ed);
	if (ExecDomain::RestrictedMode::NO_RESTRICTIONS != ed->restricted_mode_)
		throw_NO_PERMISSION ();
	return reinterpret_cast <CORBA::Nirvana::ObjectFactory::StatelessCreationFrame*> (ed->local_value_get (_get_ptr ()));
}

void ObjectFactory::stateless_creation_frame (CORBA::Nirvana::ObjectFactory::StatelessCreationFrame* scf)
{
	ExecDomain* ed = ::Nirvana::Core::Thread::current ().exec_domain ();
	assert (ed);
	if (scf)
		ed->local_value_set (_get_ptr (), scf);
	else
		ed->local_value_erase (_get_ptr ());
}

size_t ObjectFactory::offset_ptr ()
{
	CORBA::Nirvana::ObjectFactory::StatelessCreationFrame* scs = stateless_creation_frame ();
	if (scs)
		return scs->offset ();
	else {
		// Stateful object must be created in the sync doimain only.
		ExecDomain* ed = ::Nirvana::Core::Thread::current ().exec_domain ();
		if (!ed || !ed->sync_context ()->sync_domain ())
			throw_BAD_INV_ORDER ();
	}
	return 0;
}

}

extern const ::Nirvana::ImportInterfaceT <ObjectFactory> g_object_factory = {
	::Nirvana::OLF_IMPORT_INTERFACE, "CORBA/Nirvana/g_object_factory",
	ObjectFactory::repository_id_, NIRVANA_STATIC_BRIDGE (ObjectFactory, Core::ObjectFactory)
};

}
}

NIRVANA_EXPORT (_exp_CORBA_Nirvana_g_object_factory, "CORBA/Nirvana/g_object_factory", CORBA::Nirvana::ObjectFactory, CORBA::Nirvana::Core::ObjectFactory)
