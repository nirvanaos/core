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
#include <CORBA/Server.h>
#include "Binder.h"
#include "Module.h"
#include <Port/SystemInfo.h>
#include <Nirvana/OLF.h>
#include "ORB/POA.h"
#include "ORB/ServantBase.h"
#include "ORB/LocalObject.h"

namespace Nirvana {
namespace Core {

using namespace std;
using namespace CORBA;
using namespace CORBA::Nirvana;
using namespace PortableServer;
using CORBA::Nirvana::Core::POA;

ImplStatic <SyncDomain> Binder::sync_domain_;
Binder::Map Binder::map_;

class Binder::OLF_Iterator
{
public:
	OLF_Iterator (const void* address, size_t size) :
		cur_ptr_ ((OLF_Command*)address),
		end_ ((OLF_Command*)((uint8_t*)address + size))
	{
		check ();
	}

	bool end () const
	{
		return cur_ptr_ == end_;
	}

	OLF_Command* cur () const
	{
		return cur_ptr_;
	}

	void next ()
	{
		if (!end ()) {
			size_t idx = (size_t)(*cur_ptr_) - 1;
			assert (idx >= 0);
			cur_ptr_ = (OLF_Command*)((uint8_t*)cur_ptr_ + command_sizes_ [idx]);
			check ();
		}
	}

private:
	void check ();

private:
	OLF_Command* cur_ptr_;
	OLF_Command* end_;

	static const size_t command_sizes_ [OLF_IMPORT_OBJECT];
};

const size_t Binder::OLF_Iterator::command_sizes_ [OLF_IMPORT_OBJECT] = {
	sizeof (ImportInterface),
	sizeof (ExportInterface),
	sizeof (ExportObject),
	sizeof (ExportLocal),
	sizeof (ImportInterface)
};

void Binder::OLF_Iterator::check ()
{
	if (cur_ptr_ >= end_)
		cur_ptr_ = end_;
	else {
		OLF_Command cmd = *cur_ptr_;
		if (OLF_END == cmd)
			cur_ptr_ = end_;
		else if (cmd > OLF_IMPORT_OBJECT)
			throw CORBA::INITIALIZE ();
	}
}

void Binder::initialize ()
{
	Section metadata;
	if (!Port::SystemInfo::get_OLF_section (metadata))
		throw_INITIALIZE ();

	SYNC_BEGIN (&sync_domain_);
	export_add (g_binder.imp.name, g_binder.imp.itf);
	module_bind (nullptr, metadata, SyncContext::free_sync_context ());
	SYNC_END ();
}

void Binder::module_bind (Module* mod, const Section& metadata, SyncContext& sync_context)
{
	enum MetadataFlags
	{
		IMPORT_INTERFACES = 0x01,
		EXPORT_OBJECTS = 0x02,
		IMPORT_OBJECTS = 0x04
	};

	const ImportInterface* module_entry = nullptr;
	void* writable = const_cast <void*> (metadata.address);

	try {

		// Pass 1: Export pseudo objects and bind g_module.
		unsigned flags = 0;
		Key k_gmodule (g_module.imp.name);
		for (OLF_Iterator it (metadata.address, metadata.size); !it.end (); it.next ()) {
			switch (*it.cur ()) {
				case OLF_IMPORT_INTERFACE:
					if (!module_entry) {
						ImportInterface* ps = reinterpret_cast <ImportInterface*> (it.cur ());
						Key key (ps->name);
						if (key.is_a (k_gmodule)) {
							assert (mod);
							if (!k_gmodule.compatible (key))
								throw_INITIALIZE ();
							mod->module_entry_ = module_entry = ps;
							ps->itf = &mod->_get_ptr ();
							break;
						}
					}
					flags |= MetadataFlags::IMPORT_INTERFACES;
					break;

				case OLF_EXPORT_INTERFACE:
				{
					const ExportInterface* ps = reinterpret_cast <const ExportInterface*> (it.cur ());
					export_add (ps->name, ps->itf);
				}
				break;

				case OLF_EXPORT_OBJECT:
				case OLF_EXPORT_LOCAL:
					flags |= MetadataFlags::EXPORT_OBJECTS;
					break;

				case OLF_IMPORT_OBJECT:
					flags |= MetadataFlags::IMPORT_OBJECTS;
					break;

				default:
					throw_INITIALIZE ();
			}
		}

		if (!mod) {
			// Create POA
			// TODO: It is temporary solution.
			SYNC_BEGIN (nullptr);
			Servant_var <POA> poa = new POA;
			CORBA::Nirvana::Core::g_root_POA = poa->_this ();
			SYNC_END ();
		}

		if (flags) {
			writable = Port::Memory::copy (const_cast <void*> (metadata.address), const_cast <void*> (metadata.address), metadata.size, Memory::READ_WRITE);

			if (module_entry)
				mod->module_entry_ = (ImportInterface*)((uint8_t*)module_entry + ((uint8_t*)writable - (uint8_t*)metadata.address));

			// Pass 2: Import pseudo objects.
			if (flags & MetadataFlags::IMPORT_INTERFACES)
				for (OLF_Iterator it (writable, metadata.size); !it.end (); it.next ()) {
					if (OLF_IMPORT_INTERFACE == *it.cur ()) {
						ImportInterface* ps = reinterpret_cast <ImportInterface*> (it.cur ());
						if (!mod || ps != mod->module_entry_)
							ps->itf = &bind_sync (ps->name, ps->interface_id)._retn ();
					}
				}

			// Pass 3: Export objects.
			if (flags & MetadataFlags::EXPORT_OBJECTS)
				for (OLF_Iterator it (writable, metadata.size); !it.end (); it.next ()) {
					switch (*it.cur ()) {
						case OLF_EXPORT_OBJECT:
						{
							ExportObject* ps = reinterpret_cast <ExportObject*> (it.cur ());
							PortableServer::ServantBase_var core_obj = (new CORBA::Nirvana::Core::ServantBase (TypeI <PortableServer::ServantBase>::in (ps->servant_base), sync_context))->_get_ptr ();
							Object_ptr obj = AbstractBase_ptr (core_obj)->_query_interface <Object> ();
							ps->core_object = &core_obj._retn ();
							export_add (ps->name, obj);
						}
						break;

						case OLF_EXPORT_LOCAL:
						{
							ExportLocal* ps = reinterpret_cast <ExportLocal*> (it.cur ());
							LocalObject_ptr core_obj = (new CORBA::Nirvana::Core::LocalObject (TypeI <LocalObject>::in (ps->local_object), TypeI <AbstractBase>::in (ps->abstract_base), sync_context))->_get_ptr ();
							Object_ptr obj = core_obj;
							ps->core_object = &core_obj;
							export_add (ps->name, obj);
						}
						break;
					}
				}

			// Pass 4: Import objects.
			if (flags & MetadataFlags::IMPORT_OBJECTS)
				for (OLF_Iterator it (writable, metadata.size); !it.end (); it.next ()) {
					if (OLF_IMPORT_OBJECT == *it.cur ()) {
						ImportInterface* ps = reinterpret_cast <ImportInterface*> (it.cur ());
						ps->itf = &bind_sync (ps->name, ps->interface_id)._retn ();
					}
				}

			Port::Memory::copy (const_cast <void*> (metadata.address), writable, metadata.size, (writable != metadata.address) ? (Memory::READ_ONLY | Memory::RELEASE) : Memory::READ_ONLY);
			if (mod)
				mod->module_entry_ = module_entry;
		}
	} catch (...) {
		module_unbind (mod, { writable, metadata.size });
		if (writable != metadata.address)
			Port::Memory::release (writable, metadata.size);
		throw;
	}
}

void Binder::module_unbind (Module* mod, const Section& metadata) NIRVANA_NOEXCEPT
{
	// Pass 1: Release all imported interfaces.
	for (OLF_Iterator it (metadata.address, metadata.size); !it.end (); it.next ()) {
		switch (*it.cur ()) {
			case OLF_IMPORT_INTERFACE:
			case OLF_IMPORT_OBJECT:
				ImportInterface* ps = reinterpret_cast <ImportInterface*> (it.cur ());
				if (!mod || ps != mod->module_entry_)
					CORBA::Nirvana::interface_release (ps->itf);
				break;
		}
	}

	// Pass 2: Release proxies for all exported interfaces.
	for (OLF_Iterator it (metadata.address, metadata.size); !it.end (); it.next ()) {
		switch (*it.cur ()) {
			case OLF_EXPORT_INTERFACE:
			{
				const ExportInterface* ps = reinterpret_cast <const ExportInterface*> (it.cur ());
				export_remove (ps->name);
			}
			break;

			case OLF_EXPORT_OBJECT:
			{
				ExportObject* ps = reinterpret_cast <ExportObject*> (it.cur ());
				if (ps->core_object) { // Module may be bound partially because of an error.
					export_remove (ps->name);
					CORBA::Nirvana::interface_release (ps->core_object);
				}
			}
			break;

			case OLF_EXPORT_LOCAL:
			{
				ExportLocal* ps = reinterpret_cast <ExportLocal*> (it.cur ());
				if (ps->core_object) { // Module may be bound partially because of an error.
					export_remove (ps->name);
					CORBA::Nirvana::interface_release (ps->core_object);
				}
			}
			break;
		}
	}
}

void Binder::export_add (const char* name, CORBA::Nirvana::Interface_ptr itf)
{
	assert (itf);
	Key key (name);
	auto ins = map_.emplace (name, itf);
	if (!ins.second)
		throw_INV_OBJREF ();	// Duplicated name
}

void Binder::export_remove (const char* name) NIRVANA_NOEXCEPT
{
	verify (map_.erase (name));
}

inline
Interface_var Binder::bind_sync (const char* name, size_t name_len, const char* iid, size_t iid_len)
{
	Key key (name, name_len);
	auto pf = map_.lower_bound (key);
	if (pf != map_.end () && pf->first.compatible (key)) {
		Interface* itf = &pf->second;
		const StringBase <char> itf_id = itf->_epv ().interface_id;
		const StringBase <char> requested_iid (iid, iid_len);
		if (!RepositoryId::compatible (itf_id, requested_iid)) {
			AbstractBase_ptr ab = AbstractBase::_nil ();
			if (RepositoryId::compatible (itf_id, Object::repository_id_))
				ab = Object_ptr (static_cast <Object*> (itf));
			else if (RepositoryId::compatible (itf_id, AbstractBase::repository_id_))
				ab = static_cast <AbstractBase*> (itf);
			else
				throw_INV_OBJREF ();
			Interface* qi = ab->_query_interface (requested_iid);
			if (!qi)
				throw_INV_OBJREF ();
			itf = qi;
		}

		return Interface_ptr (interface_duplicate (itf));

	} else
		throw_OBJECT_NOT_EXIST ();
}

}
extern const ImportInterfaceT <Binder> g_binder = { OLF_IMPORT_INTERFACE, "Nirvana/g_binder", Binder::repository_id_, STATIC_BRIDGE (Binder, Core::Binder) };
}
