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

Binder Binder::singleton_;

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

	static const size_t command_sizes_ [OLF_MODULE_STARTUP];
};

const size_t Binder::OLF_Iterator::command_sizes_ [OLF_MODULE_STARTUP] = {
	sizeof (ImportInterface),
	sizeof (ExportInterface),
	sizeof (ExportObject),
	sizeof (ExportLocal),
	sizeof (ImportInterface),
	sizeof (ModuleStartup)
};

void Binder::OLF_Iterator::check ()
{
	if (cur_ptr_ >= end_)
		cur_ptr_ = end_;
	else {
		OLF_Command cmd = *cur_ptr_;
		if (OLF_END == cmd)
			cur_ptr_ = end_;
		else if ((size_t)cmd > countof (command_sizes_))
			invalid_metadata ();
	}
}

void Binder::initialize ()
{
	Section metadata;
	if (!Port::SystemInfo::get_OLF_section (metadata))
		throw_INITIALIZE ();

	SYNC_BEGIN (&singleton_.sync_domain_);
	singleton_.export_add (g_binder.imp.name, g_binder.imp.itf);
	singleton_.module_bind (nullptr, metadata, ModuleType::CLASS_LIBRARY);
	SYNC_END ();
}

void Binder::terminate ()
{
	CORBA::Nirvana::Core::g_root_POA = nullptr;
}

NIRVANA_NORETURN void Binder::invalid_metadata ()
{
	throw runtime_error ("Invalid file format");
}

const ModuleStartup* Binder::module_bind (Module::_ptr_type mod, const Section& metadata, ModuleType module_type)
{
	enum MetadataFlags
	{
		IMPORT_INTERFACES = 0x01,
		EXPORT_OBJECTS = 0x02,
		IMPORT_OBJECTS = 0x04
	};

	ImportInterface* module_entry = nullptr;
	void* writable = const_cast <void*> (metadata.address);
	const ModuleStartup* module_startup = nullptr;
	CoreRef <SyncDomain> sync_domain;
	SyncContext* sync_context = nullptr;

	try {

		// Pass 1: Export pseudo objects and bind g_module.
		unsigned flags = 0;
		Key k_gmodule (g_module.imp.name);
		Key k_object_factory (g_object_factory.imp.name);
		for (OLF_Iterator it (metadata.address, metadata.size); !it.end (); it.next ()) {
			switch (*it.cur ()) {
				case OLF_IMPORT_INTERFACE:
					if (!module_entry) {
						ImportInterface* ps = reinterpret_cast <ImportInterface*> (it.cur ());
						Key key (ps->name);
						if (key.is_a (k_gmodule)) {
							assert (mod);
							if (!k_gmodule.compatible (key))
								invalid_metadata ();
							module_entry = ps;
							break;
						} else if (ModuleType::EXECUTABLE == module_type && key.is_a (k_object_factory))
							invalid_metadata (); // Legacy process can not import ObjectFactory interface
					}
					flags |= MetadataFlags::IMPORT_INTERFACES;
					break;

				case OLF_EXPORT_INTERFACE: {
					if (ModuleType::EXECUTABLE == module_type) // Legacy process can not export
						invalid_metadata ();
					const ExportInterface* ps = reinterpret_cast <const ExportInterface*> (it.cur ());
					export_add (ps->name, ps->itf);
				} break;

				case OLF_EXPORT_OBJECT:
				case OLF_EXPORT_LOCAL:
					if (ModuleType::EXECUTABLE == module_type) // Legacy process can not export
						invalid_metadata ();
					flags |= MetadataFlags::EXPORT_OBJECTS;
					if (!sync_context) {
						if (ModuleType::SINGLETON == module_type) {
							sync_domain = CoreRef <SyncDomain>::create <ImplDynamic <SyncDomain> > ();
							sync_context = sync_domain;
						} else
							sync_context = &SyncContext::free_sync_context ();
					}
					break;

				case OLF_IMPORT_OBJECT:
					flags |= MetadataFlags::IMPORT_OBJECTS;
					break;

				case OLF_MODULE_STARTUP:
					if (module_startup)
						invalid_metadata (); // Must be only one startup entry
					module_startup = reinterpret_cast <const ModuleStartup*> (it.cur ());
					break;

				default:
					invalid_metadata ();
			}
		}

		if (!mod) {
			// Create POA
			// TODO: It is temporary solution.
			SYNC_BEGIN (nullptr);
			CORBA::Nirvana::Core::g_root_POA = CORBA::make_reference <POA> ()->_this ();
			SYNC_END ();
		}

		if (flags || module_entry) {
			writable = Port::Memory::copy (const_cast <void*> (metadata.address), const_cast <void*> (metadata.address), metadata.size, Memory::READ_WRITE);

			if (module_entry) {
				module_entry = (ImportInterface*)((uint8_t*)module_entry + ((uint8_t*)writable - (uint8_t*)metadata.address));
				module_entry->itf = &mod;
			}

			// Pass 2: Import pseudo objects.
			if (flags & MetadataFlags::IMPORT_INTERFACES)
				for (OLF_Iterator it (writable, metadata.size); !it.end (); it.next ()) {
					if (OLF_IMPORT_INTERFACE == *it.cur ()) {
						ImportInterface* ps = reinterpret_cast <ImportInterface*> (it.cur ());
						if (!mod || ps != module_entry)
							ps->itf = interface_duplicate (&bind_interface_sync (ps->name, ps->interface_id));
					}
				}

			// Pass 3: Export objects.
			if (flags & MetadataFlags::EXPORT_OBJECTS)
				for (OLF_Iterator it (writable, metadata.size); !it.end (); it.next ()) {
					switch (*it.cur ()) {
						case OLF_EXPORT_OBJECT: {
							ExportObject* ps = reinterpret_cast <ExportObject*> (it.cur ());
							PortableServer::ServantBase::_ptr_type core_obj = (new CORBA::Nirvana::Core::ServantBase (Type <PortableServer::ServantBase>::in (ps->servant_base), *sync_context))->_get_ptr ();
							Object::_ptr_type obj = AbstractBase::_ptr_type (core_obj)->_query_interface <Object> ();
							ps->core_object = &core_obj;
							export_add (ps->name, obj);
						} break;

						case OLF_EXPORT_LOCAL: {
							ExportLocal* ps = reinterpret_cast <ExportLocal*> (it.cur ());
							LocalObject::_ptr_type core_obj = (new CORBA::Nirvana::Core::LocalObject (Type <LocalObject>::in (ps->local_object), Type <AbstractBase>::in (ps->abstract_base), *sync_context))->_get_ptr ();
							Object::_ptr_type obj = core_obj;
							ps->core_object = &core_obj;
							export_add (ps->name, obj);
						} break;
					}
				}

			// Pass 4: Import objects.
			if (flags & MetadataFlags::IMPORT_OBJECTS)
				for (OLF_Iterator it (writable, metadata.size); !it.end (); it.next ()) {
					if (OLF_IMPORT_OBJECT == *it.cur ()) {
						ImportInterface* ps = reinterpret_cast <ImportInterface*> (it.cur ());
						Object::_ptr_type obj = bind_sync (ps->name);
						const StringBase <char> requested_iid (ps->interface_id);
						InterfacePtr itf;
						if (RepositoryId::compatible (obj->_epv ().header.interface_id, requested_iid))
							itf = &obj;
						else {
							itf = AbstractBase::_ptr_type (obj)->_query_interface (requested_iid);
							if (!itf)
								throw_INV_OBJREF ();
						}
						ps->itf = interface_duplicate (&itf);
					}
				}

			Port::Memory::copy (const_cast <void*> (metadata.address), writable, metadata.size, (writable != metadata.address) ? (Memory::READ_ONLY | Memory::SRC_RELEASE) : Memory::READ_ONLY);
		}
	} catch (...) {
		module_unbind (mod, { writable, metadata.size });
		if (writable != metadata.address)
			Port::Memory::release (writable, metadata.size);
		throw;
	}

	return module_startup;
}

void Binder::module_unbind (Module::_ptr_type mod, const Section& metadata) NIRVANA_NOEXCEPT
{
	// Pass 1: Remove all exports from table.
	// This pass will not cause inter-domain calls.
	for (OLF_Iterator it (metadata.address, metadata.size); !it.end (); it.next ()) {
		switch (*it.cur ()) {
			case OLF_EXPORT_INTERFACE: {
				const ExportInterface* ps = reinterpret_cast <const ExportInterface*> (it.cur ());
				export_remove (ps->name);
			} break;

			case OLF_EXPORT_OBJECT: {
				ExportObject* ps = reinterpret_cast <ExportObject*> (it.cur ());
				if (ps->core_object) // Module may be bound partially because of an error.
					export_remove (ps->name);
			} break;

			case OLF_EXPORT_LOCAL: {
				ExportLocal* ps = reinterpret_cast <ExportLocal*> (it.cur ());
				if (ps->core_object) // Module may be bound partially because of an error.
					export_remove (ps->name);
			} break;
		}
	}
	
	// Pass 2: Release all imported interfaces.
	for (OLF_Iterator it (metadata.address, metadata.size); !it.end (); it.next ()) {
		switch (*it.cur ()) {
			case OLF_IMPORT_INTERFACE:
			case OLF_IMPORT_OBJECT: {
				ImportInterface* ps = reinterpret_cast <ImportInterface*> (it.cur ());
				if (!mod || ps->itf != &mod)
					CORBA::Nirvana::interface_release (ps->itf);
			} break;
		}
	}

	// Pass 3: Release proxies for all exported objects.
	for (OLF_Iterator it (metadata.address, metadata.size); !it.end (); it.next ()) {
		switch (*it.cur ()) {
			case OLF_EXPORT_OBJECT: {
				ExportObject* ps = reinterpret_cast <ExportObject*> (it.cur ());
				if (ps->core_object) // Module may be bound partially because of an error.
					CORBA::Nirvana::interface_release (ps->core_object);
			} break;

			case OLF_EXPORT_LOCAL: {
				ExportLocal* ps = reinterpret_cast <ExportLocal*> (it.cur ());
				if (ps->core_object) // Module may be bound partially because of an error.
					CORBA::Nirvana::interface_release (ps->core_object);
			} break;
		}
	}
}

void Binder::export_add (const char* name, InterfacePtr itf)
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

Binder::InterfacePtr Binder::find (const char* name, size_t name_len) const
{
	Key key (name, name_len);
	auto pf = map_.lower_bound (key);
	if (pf != map_.end () && pf->first.compatible (key)) {
		return pf->second;
	}
	return nullptr;
}

Binder::InterfacePtr Binder::bind_interface_sync (const char* name, size_t name_len, const char* iid, size_t iid_len)
{
	Interface::_ptr_type itf = find (name, name_len);
	if (itf) {
		const StringBase <char> itf_id = itf->_epv ().interface_id;
		const StringBase <char> requested_iid (iid, iid_len);
		if (!RepositoryId::compatible (itf_id, requested_iid)) {
			AbstractBase::_ptr_type ab = AbstractBase::_nil ();
			if (RepositoryId::compatible (itf_id, Object::repository_id_))
				ab = Object::_ptr_type (static_cast <Object*> (&itf));
			else if (RepositoryId::compatible (itf_id, AbstractBase::repository_id_))
				ab = static_cast <AbstractBase*> (&itf);
			else
				throw_INV_OBJREF ();
			InterfacePtr qi = ab->_query_interface (requested_iid);
			if (!qi)
				throw_INV_OBJREF ();
			itf = qi;
		}

		return itf;

	} else
		throw_OBJECT_NOT_EXIST ();
}

CORBA::Object::_ptr_type Binder::bind_sync (const char* name, size_t name_len)
{
	Interface::_ptr_type itf = find (name, name_len);
	if (itf) {
		if (RepositoryId::compatible (itf->_epv ().interface_id, Object::repository_id_))
			return static_cast <Object*> (&itf);
		else
			throw_INV_OBJREF ();
	} else
		throw_OBJECT_NOT_EXIST ();
}

}

extern const ImportInterfaceT <Binder> g_binder = { OLF_IMPORT_INTERFACE, "Nirvana/g_binder", Binder::repository_id_, NIRVANA_STATIC_BRIDGE (Binder, Core::Binder) };

}
