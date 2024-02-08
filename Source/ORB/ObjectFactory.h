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
#include "../WaitableRef.h"

namespace CORBA {
namespace Core {

/// Implementation of CORBA::Internal::ObjectFactory.
class ObjectFactory :
	public servant_traits <Internal::ObjectFactory>::ServantStatic <ObjectFactory>
{
	static const TimeBase::TimeT PROXY_CREATION_DEADLINE = 10 * TimeBase::MILLISECOND;

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

	static void create_servant (PortableServer::Servant servant, void* ref, Object::_ptr_type comp)
	{
		create <PortableServer::ServantBase, PortableServer::Core::ServantBase> (servant, ref, comp);
	}

	static void create_local_object (CORBA::LocalObject::_ptr_type servant, void* ref, Object::_ptr_type comp)
	{
		create <CORBA::LocalObject, CORBA::Core::LocalObject> (servant, ref, comp);
	}

	static void stateless_begin (StatelessCreationFrame& scf)
	{
		if (!(scf.tmp () && scf.size ()))
			throw BAD_PARAM ();
		Nirvana::Core::Heap& sm = stateless_memory ();
		void* p = sm.allocate (0, scf.size (), Nirvana::Memory::READ_ONLY | Nirvana::Memory::RESERVED);
		scf.offset ((uint8_t*)p - (uint8_t*)scf.tmp ());
		scf.memory (&sm);
		Nirvana::Core::ExecDomain& ed = Nirvana::Core::ExecDomain::current ();
		scf.next (ed.TLS_get (Nirvana::Core::CoreTLS::CORE_TLS_OBJECT_FACTORY));
		ed.TLS_set (Nirvana::Core::CoreTLS::CORE_TLS_OBJECT_FACTORY, &scf);
	}

	static void* stateless_end (bool success)
	{
		Nirvana::Core::ExecDomain& ed = Nirvana::Core::ExecDomain::current ();
		StatelessCreationFrame* scf = (StatelessCreationFrame*)ed.TLS_get (Nirvana::Core::CoreTLS::CORE_TLS_OBJECT_FACTORY);
		if (!scf)
			throw BAD_INV_ORDER ();
		void* p = (Octet*)scf->tmp () + scf->offset ();
		Nirvana::Core::Heap& sm = *(Nirvana::Core::Heap*)scf->memory ();
		if (success) {
			if (scf->proxy ())
				((ServantProxyBase*)scf->proxy ())->apply_offset (scf->offset ());
			sm.copy (p, const_cast <void*> (scf->tmp ()), scf->size (), Nirvana::Memory::READ_ONLY);
		} else {
			sm.release (p, scf->size ());
			p = nullptr;
		}
		ed.TLS_set (Nirvana::Core::CoreTLS::CORE_TLS_OBJECT_FACTORY, scf->next ());
		return p;
	}

	static const void* stateless_copy (const void* src, size_t size)
	{
		return stateless_memory ().copy (nullptr, const_cast <void*> (src), size, Nirvana::Memory::READ_ONLY);
	}

	static bool is_free_sync_context () noexcept
	{
		return Nirvana::Core::SyncContext::current ().is_free_sync_context ();
	}

	static StatelessCreationFrame* begin_proxy_creation (const Internal::Interface* servant);

private:
	typedef Nirvana::Core::WaitableRef <Internal::Interface*> WaitableRef;

	static Nirvana::Core::Heap& stateless_memory ();

	template <class ServantInterface, class ServantObject>
	static void create (Internal::I_ptr <ServantInterface> servant, void* pref, Object::_ptr_type comp)
	{
		if (!servant || !pref)
			throw BAD_PARAM ();

		Internal::Interface*& iref = *reinterpret_cast <Internal::Interface**> (pref);
		uintptr_t ref_bits = (uintptr_t)iref;
		if ((ref_bits & ~1) && !(ref_bits & 1)) {
			assert (false); // Must be checked at the user side
			return; // Already created
		}

		StatelessCreationFrame* scf = begin_proxy_creation (&servant);
		ServantObject* proxy = nullptr;
		if ((ref_bits & 1) && Nirvana::Core::SyncContext::current ().sync_domain ()) {
			if (ref_bits == 1)
				iref = nullptr;
			WaitableRef& wref = *reinterpret_cast <WaitableRef*> (pref);
			if (wref.initialize (PROXY_CREATION_DEADLINE)) {
				auto wait_list = wref.wait_list ();
				try {
					wref = proxy = ServantObject::create (servant, comp);
				} catch (...) {
					wait_list->on_exception ();
					throw;
				}
			} else
				wref.wait_list ()->wait ();
		} else {
			assert ((ref_bits & ~1) == 0);
			iref = proxy = ServantObject::create (servant, comp);
		}

		if (scf && proxy)
			scf->proxy (&static_cast <ServantProxyBase&> (proxy->proxy ()));
	}
};

}
}

#endif
