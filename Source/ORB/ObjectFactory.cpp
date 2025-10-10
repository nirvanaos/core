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

using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {
namespace Internal {

ObjectFactory::StatelessCreationFrame* Static_object_factory::begin_proxy_creation (const Internal::Interface* servant)
{
	ExecDomain* ed = ::Nirvana::Core::Thread::current ().exec_domain ();
	if (!ed)
		throw BAD_INV_ORDER ();

	switch (ed->sync_context ().sync_context_type ()) {
	case SyncContext::Type::FREE_MODULE_TERM:
	case SyncContext::Type::SINGLETON_TERM:
		throw NO_PERMISSION ();
	}

	// Check for the stateless creation.
	// If scf established and servant pointer is inside the object, then it is stateless object creation.
	StatelessCreationFrame* scf = (StatelessCreationFrame*)ed->TLS_get (CoreTLS::CORE_TLS_OBJECT_FACTORY);
	if (!scf || servant < scf->tmp () || ((const Octet*)scf->tmp () + scf->size ()) <= (const Octet*)servant)
		return nullptr; // Not a stateless object.
	else
		return scf; // A stateless object
}

Heap& Static_object_factory::stateless_memory ()
{
	SyncContext& sc = SyncContext::current ();
	Heap* heap = sc.stateless_memory ();
	if (!heap) {
		SyncDomain* sd = sc.sync_domain ();
		if (sd)
			heap = &sd->mem_context ().heap ();
		else
			throw BAD_OPERATION ();
	}
	return *heap;
}

}
}
