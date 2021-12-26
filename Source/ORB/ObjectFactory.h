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

#include <CORBA/Server.h>
#include <ObjectFactory_s.h>
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
		return Nirvana::Core::SyncDomain::enter ().heap ().allocate (0, size, 0);
	}

	static void memory_release (void* p, size_t size)
	{
		// In sync domain: release from the current sync domain heap.
		Nirvana::Core::SyncDomain* sd = Nirvana::Core::SyncContext::current ().sync_domain ();
		if (sd)
			sd->heap ().release (p, size);
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

	void stateless_begin (CORBA::Internal::ObjectFactory::StatelessCreationFrame& scs)
	{
		if (!(scs.tmp () && scs.size ()))
			throw BAD_PARAM ();
		Nirvana::Core::Heap* stateless_memory = Nirvana::Core::SyncContext::current ().stateless_memory ();
		if (!stateless_memory)
			throw BAD_OPERATION ();
		void* p = stateless_memory->allocate (0, scs.size (), Nirvana::Memory::READ_ONLY | Nirvana::Memory::RESERVED);
		scs.offset ((uint8_t*)p - (uint8_t*)scs.tmp ());
		stateless_creation_frame (&scs);
	}

	void* stateless_end (bool success)
	{
		StatelessCreationFrame* scs = stateless_creation_frame ();
		if (!scs)
			throw BAD_INV_ORDER ();
		stateless_creation_frame (nullptr);
		void* p = (Octet*)scs->tmp () + scs->offset ();
		Nirvana::Core::Heap* stateless_memory = Nirvana::Core::SyncContext::current ().stateless_memory ();
		assert (stateless_memory);
		if (success) {
			stateless_memory->copy (p, const_cast <void*> (scs->tmp ()), scs->size (), Nirvana::Memory::READ_ONLY);
			return p;
		} else {
			stateless_memory->release (p, scs->size ());
			return nullptr;
		}
	}

	static I_ref <CORBA::Internal::ReferenceCounter> create_reference_counter (CORBA::Internal::DynamicServant::_ptr_type dynamic)
	{
		return make_pseudo <ReferenceCounter> (offset_ptr (dynamic));
	}

	static I_ref <PortableServer::ServantBase> create_servant (PortableServer::Servant servant)
	{
		return make_pseudo <ServantBase> (offset_ptr (servant));
	}

	static I_ref <CORBA::LocalObject> create_local_object (I_ptr <CORBA::LocalObject> servant, I_ptr <AbstractBase> abstract_base)
	{
		return make_pseudo <LocalObject> (offset_ptr (servant), offset_ptr (abstract_base));
	}

private:
	static CORBA::Internal::ObjectFactory::StatelessCreationFrame* stateless_creation_frame ();
	static void stateless_creation_frame (CORBA::Internal::ObjectFactory::StatelessCreationFrame*);

	template <class I>
	static I_ptr <I> offset_ptr (I_ptr <I> p)
	{
		return reinterpret_cast <I*> ((Octet*)&p + offset_ptr ());
	}

	static size_t offset_ptr ();
};

}
}
}

#endif
