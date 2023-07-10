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
#ifndef NIRVANA_ORB_CORE_OBJECTFACTORY_H_
#define NIRVANA_ORB_CORE_OBJECTFACTORY_H_
#pragma once

#include <CORBA/Server.h>
#include "ServantBase.h"
#include "LocalObject.h"
#include "SyncDomain.h"
#include <CORBA/ObjectFactory_s.h>
#include <CORBA/I_var.h>

namespace CORBA {
namespace Core {

/// Implementation of CORBA::Internal::ObjectFactory.
class ObjectFactory :
	public servant_traits <Internal::ObjectFactory>::ServantStatic <ObjectFactory>
{
public:
	static void* memory_allocate (size_t size)
	{
		// Enter sync domain and allocate from the sync domain heap.
		return Nirvana::Core::SyncDomain::enter ().mem_context ().heap ().allocate (0, size, 0);
	}

	static void memory_release (void* p, size_t size)
	{
		// In sync domain: release from the current sync domain heap.
		Nirvana::Core::SyncDomain* sd = Nirvana::Core::SyncContext::current ().sync_domain ();
		if (sd)
			sd->mem_context ().heap ().release (p, size);
		else {
			// Release from the read-only heap
			Nirvana::Core::Heap* stateless_memory = Nirvana::Core::SyncContext::current ().stateless_memory ();
			assert (stateless_memory);
			stateless_memory->release (p, size);
		}
	}

	static PortableServer::ServantBase::_ref_type create_servant (PortableServer::Servant servant)
	{
		Frame frame (&servant);
		return Internal::I_var <PortableServer::ServantBase> (PortableServer::Core::ServantBase::create (servant));
	}

	static CORBA::LocalObject::_ref_type create_local_object (CORBA::LocalObject::_ptr_type servant)
	{
		Frame frame (&servant);
		return Internal::I_var <CORBA::LocalObject> (CORBA::Core::LocalObject::create (servant));
	}

	void stateless_begin (StatelessCreationFrame& scf)
	{
		if (!(scf.tmp () && scf.size ()))
			throw BAD_PARAM ();
		void* p = stateless_memory ().allocate (0, scf.size (), Nirvana::Memory::READ_ONLY | Nirvana::Memory::RESERVED);
		scf.offset ((uint8_t*)p - (uint8_t*)scf.tmp ());
		Nirvana::Core::ExecDomain& ed = Nirvana::Core::ExecDomain::current ();
		scf.next (ed.TLS_get (Nirvana::Core::CoreTLS::CORE_TLS_OBJECT_FACTORY));
		ed.TLS_set (Nirvana::Core::CoreTLS::CORE_TLS_OBJECT_FACTORY, &scf);
	}

	void* stateless_end (bool success)
	{
		Nirvana::Core::ExecDomain& ed = Nirvana::Core::ExecDomain::current ();
		StatelessCreationFrame* scf = (StatelessCreationFrame*)ed.TLS_get (Nirvana::Core::CoreTLS::CORE_TLS_OBJECT_FACTORY);
		if (!scf)
			throw BAD_INV_ORDER ();
		ed.TLS_set (Nirvana::Core::CoreTLS::CORE_TLS_OBJECT_FACTORY, scf->next ());
		void* p = (Octet*)scf->tmp () + scf->offset ();
		Nirvana::Core::Heap& sm = stateless_memory ();
		if (success) {
			sm.copy (p, const_cast <void*> (scf->tmp ()), scf->size (), Nirvana::Memory::READ_ONLY);
			return p;
		} else {
			sm.release (p, scf->size ());
			return nullptr;
		}
	}

	static const void* stateless_copy (const void* src, size_t size)
	{
		return stateless_memory ().copy (nullptr, const_cast <void*> (src), size, Nirvana::Memory::READ_ONLY);
	}

	static bool is_free_sync_context ()
	{
		return Nirvana::Core::SyncContext::current ().is_free_sync_context ();
	}

private:
	class Frame : public StatelessCreationFrame
	{
	public:
		Frame (const Internal::Interface* servant);
		~Frame ();

	private:
		bool pop_;
	};

	static Nirvana::Core::Heap& stateless_memory ();
};

}
}

#endif
