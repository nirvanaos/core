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
#include "ObjectFactory.h"

using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {
namespace Core {

ObjectFactory::Frame::Frame (const Internal::Interface* servant) :
	StatelessCreationFrame (nullptr, 0, 0, nullptr),
	pop_ (false)
{
	ExecDomain* ed = ::Nirvana::Core::Thread::current ().exec_domain ();
	if (!ed)
		throw_BAD_INV_ORDER ();

	if (ExecDomain::RestrictedMode::MODULE_TERMINATE == ed->restricted_mode ())
		throw_NO_PERMISSION ();

	StatelessCreationFrame* scf = (StatelessCreationFrame*)ed->TLS_get (CoreTLS::CORE_TLS_OBJECT_FACTORY);
	// Check for the stateless creation.
	// If scf established and servant pointer is inside the object, then it is stateless object creation.
	if (!scf || servant < scf->tmp () || ((const Octet*)scf->tmp () + scf->size ()) <= (const Octet*)servant) {
		// It is not a stateless object, we must establish a new frame.
		next (ed->TLS_get (CoreTLS::CORE_TLS_OBJECT_FACTORY));
		ed->TLS_set (CoreTLS::CORE_TLS_OBJECT_FACTORY, static_cast <StatelessCreationFrame*> (this));
		pop_ = true;
	}
}

ObjectFactory::Frame::~Frame ()
{
	if (pop_)
		ExecDomain::current ().TLS_set (CoreTLS::CORE_TLS_OBJECT_FACTORY, next ());
}

Heap& ObjectFactory::stateless_memory ()
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
