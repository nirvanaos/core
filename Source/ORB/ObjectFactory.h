/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
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

#include <CORBA/CORBA.h>
#include <CORBA/ObjectFactory_s.h>
#include "ServantBase.h"
#include "LocalObject.h"
#include "ReferenceCounter.h"
#include "SyncDomain.h"

namespace CORBA {
namespace Nirvana {
namespace Core {

class ObjectFactory :
	public ServantStatic <ObjectFactory, CORBA::Nirvana::ObjectFactory>
{
public:
	static void* memory_allocate (size_t size)
	{
		::Nirvana::Core::SyncDomain::enter ();
		// TODO: Allocate from the current sync domain heap.
		return ::Nirvana::Core::g_core_heap->allocate (0, size, 0);
	}

	static void memory_release (void* p, size_t size)
	{
		// TODO: In sync domain: release from the current sync domain heap.
		// Otherwise: release from the read-only heap.
		::Nirvana::Core::g_core_heap->release (p, size);
	}

	void stateless_begin (StatelessCreationFrame& scs)
	{
		if (!(scs.tmp && scs.size))
			throw BAD_PARAM ();
		// TODO: Allocate from read-only heap
		void* p = ::Nirvana::Core::g_core_heap->allocate (0, scs.size, ::Nirvana::Memory::READ_ONLY | ::Nirvana::Memory::RESERVED);
		scs.offset = (uint8_t*)p - (uint8_t*)scs.tmp;
		stateless_creation_frame () = &scs;
	}

	void* stateless_end (bool success)
	{
		StatelessCreationFrame*& scsr = stateless_creation_frame ();
		StatelessCreationFrame* scs = scsr;
		if (!scs)
			throw BAD_INV_ORDER ();
		void* p = (Octet*)scs->tmp + scs->offset;
		// TODO: Allocate from read-only heap
		if (success) {
			::Nirvana::Core::g_core_heap->copy (p, const_cast <void*> (scs->tmp), scs->size, ::Nirvana::Memory::READ_ONLY);
			scsr = nullptr;
			return p;
		} else {
			scsr = nullptr;
			::Nirvana::Core::g_core_heap->release (p, scs->size);
			return nullptr;
		}
	}

	static CORBA::Nirvana::ReferenceCounter_ptr create_reference_counter (CORBA::Nirvana::DynamicServant_ptr dynamic)
	{
		return (new ReferenceCounter (offset_ptr (dynamic)))->_get_ptr ();
	}

	static PortableServer::ServantBase_var create_servant (PortableServer::Servant servant)
	{
		return (new ServantBase (offset_ptr (servant)))->_get_ptr ();
	}

	static LocalObject_var create_local_object (LocalObject_ptr servant, AbstractBase_ptr abstract_base)
	{
		return (new LocalObject (offset_ptr (servant), offset_ptr (abstract_base)))->_get_ptr ();
	}

private:
	static StatelessCreationFrame*& stateless_creation_frame ();

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
