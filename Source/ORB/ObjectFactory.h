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
#include "IDL/ObjectFactory_s.h"
#include "ServantBase.h"
#include "LocalObject.h"
#include "ReferenceCounter.h"
#include "SyncDomain.h"

namespace CORBA {
namespace Internal {
namespace Core {

class ObjectFactory :
	public ServantStatic <ObjectFactory, CORBA::Internal::ObjectFactory>
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

	static bool stateless_available ()
	{
		return Nirvana::Core::SyncContext::current ().stateless_memory () != nullptr;
	}

	void stateless_begin (CORBA::Internal::ObjectFactory::StatelessCreationFrame& scf)
	{
		if (!(scf.tmp () && scf.size ()))
			throw BAD_PARAM ();
		Nirvana::Core::Heap* stateless_memory = Nirvana::Core::SyncContext::current ().stateless_memory ();
		if (!stateless_memory)
			throw BAD_OPERATION ();
		void* p = stateless_memory->allocate (0, scf.size (), Nirvana::Memory::READ_ONLY | Nirvana::Memory::RESERVED);
		scf.offset ((uint8_t*)p - (uint8_t*)scf.tmp ());
		Nirvana::Core::TLS& tls = Nirvana::Core::TLS::current ();
		scf.next (tls.get (Nirvana::Core::TLS::CORE_TLS_OBJECT_FACTORY));
		tls.set (Nirvana::Core::TLS::CORE_TLS_OBJECT_FACTORY, &scf);
	}

	void* stateless_end (bool success)
	{
		Nirvana::Core::TLS& tls = Nirvana::Core::TLS::current ();
		StatelessCreationFrame* scf = (StatelessCreationFrame*)tls.get (Nirvana::Core::TLS::CORE_TLS_OBJECT_FACTORY);
		if (!scf)
			throw BAD_INV_ORDER ();
		tls.set (Nirvana::Core::TLS::CORE_TLS_OBJECT_FACTORY, scf->next ());
		void* p = (Octet*)scf->tmp () + scf->offset ();
		Nirvana::Core::Heap* stateless_memory = Nirvana::Core::SyncContext::current ().stateless_memory ();
		assert (stateless_memory);
		if (success) {
			stateless_memory->copy (p, const_cast <void*> (scf->tmp ()), scf->size (), Nirvana::Memory::READ_ONLY);
			return p;
		} else {
			stateless_memory->release (p, scf->size ());
			return nullptr;
		}
	}

	static I_ref <CORBA::Internal::ReferenceCounter> create_reference_counter (CORBA::Internal::DynamicServant::_ptr_type dynamic)
	{
		Frame frame;
		return make_pseudo <ReferenceCounter> (dynamic);
	}

	static I_ref <PortableServer::ServantBase> create_servant (PortableServer::Servant servant)
	{
		Frame frame;
		return make_pseudo <PortableServer::Core::ServantBase> (servant);
	}

	static I_ref <CORBA::LocalObject> create_local_object (I_ptr <CORBA::LocalObject> servant, I_ptr <AbstractBase> abstract_base)
	{
		Frame frame;
		return make_pseudo <LocalObject> (servant, abstract_base);
	}

private:
	class Frame : public StatelessCreationFrame
	{
	public:
		Frame ();
		~Frame ();

	private:
		bool pop_;
	};

};

}
}
}

#endif
