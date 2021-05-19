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
#include <Nirvana/OLF_Iterator.h>
#include "ORB/POA.h"
#include "ORB/ServantBase.h"
#include "ORB/LocalObject.h"
#include "ClassLibrary.h"
#include "Singleton.h"
#include "Legacy/Executable.h"
#include "Loader.h"

namespace Nirvana {
namespace Core {

using namespace std;
using namespace CORBA;
using namespace CORBA::Internal;
using namespace PortableServer;
using CORBA::Internal::Core::POA;

Binder Binder::singleton_;

void Binder::Map::insert (const char* name, InterfacePtr itf)
{
	assert (itf);
	if (!MapBase::emplace (name, itf).second)
		throw_INV_OBJREF ();	// Duplicated name
}

void Binder::Map::erase (const char* name) NIRVANA_NOEXCEPT
{
	verify (MapBase::erase (name));
}

void Binder::Map::merge (const Map& src)
{
	for (auto it = src.begin (); it != src.end (); ++it) {
		if (!MapBase::insert (*it).second) {
			for (auto it1 = src.begin (); it1 != it; ++it1) {
				MapBase::erase (it1->first);
			}
			throw_INV_OBJREF ();	// Duplicated name
		}
	}
}

Binder::InterfacePtr Binder::Map::find (const Key& key) const
{
	auto pf = lower_bound (key);
	if (pf != end () && pf->first.compatible (key)) {
		return pf->second;
	}
	return nullptr;
}

void Binder::initialize ()
{
	Section metadata;
	if (!Port::SystemInfo::get_OLF_section (metadata))
		throw_INITIALIZE ();

	SYNC_BEGIN (&singleton_.sync_domain_);
	ModuleContext context{ SyncContext::free_sync_context () };
	singleton_.module_bind (nullptr, metadata, &context);
	singleton_.map_.merge (context.exports);
	SYNC_END ();
}

void Binder::terminate ()
{
	CORBA::Internal::Core::g_root_POA = nullptr;
}

NIRVANA_NORETURN void Binder::invalid_metadata ()
{
	throw runtime_error ("Invalid file format");
}

const ModuleStartup* Binder::module_bind (::Nirvana::Module::_ptr_type mod, const Section& metadata, ModuleContext* mod_context) const
{
	ExecDomain* exec_domain = Thread::current ().exec_domain ();
	assert (exec_domain);
	void* prev_context = exec_domain->binder_context_;
	exec_domain->binder_context_ = mod_context;

	enum MetadataFlags
	{
		IMPORT_INTERFACES = 0x01,
		EXPORT_OBJECTS = 0x02,
		IMPORT_OBJECTS = 0x04
	};

	ImportInterface* module_entry = nullptr;
	void* writable = const_cast <void*> (metadata.address);
	const ModuleStartup* module_startup = nullptr;

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
						} else if (!mod_context && key.is_a (k_object_factory))
							invalid_metadata (); // Legacy process can not import ObjectFactory interface
					}
					flags |= MetadataFlags::IMPORT_INTERFACES;
					break;

				case OLF_EXPORT_INTERFACE: {
					if (!mod_context) // Legacy process can not export
						invalid_metadata ();
					const ExportInterface* ps = reinterpret_cast <const ExportInterface*> (it.cur ());
					mod_context->exports.insert (ps->name, ps->itf);
				} break;

				case OLF_EXPORT_OBJECT:
				case OLF_EXPORT_LOCAL:
					if (!mod_context) // Legacy process can not export
						invalid_metadata ();
					flags |= MetadataFlags::EXPORT_OBJECTS;
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
			CORBA::Internal::Core::g_root_POA = CORBA::make_reference <POA> ()->_this ();
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
							reinterpret_cast <InterfaceRef&> (ps->itf) = bind_interface_sync (ps->name, ps->interface_id);
					}
				}

			// Pass 3: Export objects.
			if (flags & MetadataFlags::EXPORT_OBJECTS) {
				assert (mod_context); // Legacy executable can not export.
				for (OLF_Iterator it (writable, metadata.size); !it.end (); it.next ()) {
					switch (*it.cur ()) {
						case OLF_EXPORT_OBJECT: {
							ExportObject* ps = reinterpret_cast <ExportObject*> (it.cur ());
							PortableServer::ServantBase::_ptr_type core_obj = (
								new CORBA::Internal::Core::ServantBase (Type <PortableServer::ServantBase>::in (ps->servant_base),
									mod_context->sync_context))->_get_ptr ();
							Object::_ptr_type obj = AbstractBase::_ptr_type (core_obj)->_query_interface <Object> ();
							ps->core_object = &core_obj;
							mod_context->exports.insert (ps->name, obj);
						} break;

						case OLF_EXPORT_LOCAL: {
							ExportLocal* ps = reinterpret_cast <ExportLocal*> (it.cur ());
							LocalObject::_ptr_type core_obj = (
								new CORBA::Internal::Core::LocalObject (Type <LocalObject>::in (ps->local_object),
									Type <AbstractBase>::in (ps->abstract_base), mod_context->sync_context))->_get_ptr ();
							Object::_ptr_type obj = core_obj;
							ps->core_object = &core_obj;
							mod_context->exports.insert (ps->name, obj);
						} break;
					}
				}
			}

			// Pass 4: Import objects.
			if (flags & MetadataFlags::IMPORT_OBJECTS)
				for (OLF_Iterator it (writable, metadata.size); !it.end (); it.next ()) {
					if (OLF_IMPORT_OBJECT == *it.cur ()) {
						ImportInterface* ps = reinterpret_cast <ImportInterface*> (it.cur ());
						Object::_ref_type obj = bind_sync (ps->name);
						const StringBase <char> requested_iid (ps->interface_id);
						if (RepositoryId::compatible (obj->_epv ().header.interface_id, requested_iid))
							reinterpret_cast <Object::_ref_type&> (ps->itf) = move (obj);
						else {
							InterfacePtr itf = AbstractBase::_ptr_type (obj)->_query_interface (requested_iid);
							if (!itf)
								throw_INV_OBJREF ();
							ps->itf = interface_duplicate (&itf);
						}
					}
				}

			Port::Memory::copy (const_cast <void*> (metadata.address), writable, metadata.size, (writable != metadata.address) ? (Memory::READ_ONLY | Memory::SRC_RELEASE) : Memory::READ_ONLY);
		}
	} catch (...) {
		module_unbind (mod, { writable, metadata.size });
		if (writable != metadata.address)
			Port::Memory::release (writable, metadata.size);
		exec_domain->binder_context_ = nullptr;
		throw;
	}

	exec_domain->binder_context_ = prev_context;

	return module_startup;
}

inline
void Binder::remove_module_exports (const Section& metadata)
{
	// Pass 1: Remove all exports from the map.
	// This pass will not cause inter-domain calls.
	for (OLF_Iterator it (metadata.address, metadata.size); !it.end (); it.next ()) {
		switch (*it.cur ()) {
			case OLF_EXPORT_INTERFACE: {
				const ExportInterface* ps = reinterpret_cast <const ExportInterface*> (it.cur ());
				map_.erase (ps->name);
			} break;

			case OLF_EXPORT_OBJECT: {
				ExportObject* ps = reinterpret_cast <ExportObject*> (it.cur ());
				map_.erase (ps->name);
			} break;

			case OLF_EXPORT_LOCAL: {
				ExportLocal* ps = reinterpret_cast <ExportLocal*> (it.cur ());
				map_.erase (ps->name);
			} break;
		}
	}
}

void Binder::unbind (Module& mod) NIRVANA_NOEXCEPT
{
	SYNC_BEGIN (&singleton_.sync_domain_);
	singleton_.remove_module_exports (mod.metadata ());
	singleton_.module_unbind (mod._get_ptr (), mod.metadata ());
	SYNC_END ();
}

void Binder::module_unbind (::Nirvana::Module::_ptr_type mod, const Section& metadata) NIRVANA_NOEXCEPT
{
	// Pass 1: Release all imported interfaces.
	for (OLF_Iterator it (metadata.address, metadata.size); !it.end (); it.next ()) {
		switch (*it.cur ()) {
			case OLF_IMPORT_INTERFACE:
			case OLF_IMPORT_OBJECT: {
				ImportInterface* ps = reinterpret_cast <ImportInterface*> (it.cur ());
				if (!mod || ps->itf != &mod)
					CORBA::Internal::interface_release (ps->itf);
			} break;
		}
	}

	// Pass 2: Release proxies for all exported objects.
	for (OLF_Iterator it (metadata.address, metadata.size); !it.end (); it.next ()) {
		switch (*it.cur ()) {
			case OLF_EXPORT_OBJECT: {
				ExportObject* ps = reinterpret_cast <ExportObject*> (it.cur ());
				CORBA::Internal::interface_release (ps->core_object);
			} break;

			case OLF_EXPORT_LOCAL: {
				ExportLocal* ps = reinterpret_cast <ExportLocal*> (it.cur ());
				CORBA::Internal::interface_release (ps->core_object);
			} break;
		}
	}
}

Binder::InterfaceRef Binder::find (const Key& name) const
{
	const ExecDomain* exec_domain = Thread::current ().exec_domain ();
	assert (exec_domain);
	if (ExecDomain::RestrictedMode::MODULE_TERMINATE == exec_domain->restricted_mode_)
		throw_NO_PERMISSION ();
	ModuleContext* context = reinterpret_cast <ModuleContext*> (exec_domain->binder_context_);
	InterfaceRef itf;
	if (context)
		itf = context->exports.find (name);
	if (!itf) {
		itf = map_.find (name);
		if (!itf) {
			CoreRef <Module> mod = Loader::load ("TestModule.olf", false);
			itf = map_.find (name);
			if (!itf)
				throw_OBJECT_NOT_EXIST ();
		}
	}
	return itf;
}

Binder::InterfaceRef Binder::bind_interface_sync (const Key& name, String_in iid) const
{
	InterfaceRef itf = find (name);
	StringBase <char> itf_id = itf->_epv ().interface_id;
	if (!RepositoryId::compatible (itf_id, iid)) {
		AbstractBase::_ptr_type ab = AbstractBase::_nil ();
		if (RepositoryId::compatible (itf_id, Object::repository_id_))
			ab = Object::_ptr_type (static_cast <Object*> (&InterfacePtr (itf)));
		else if (RepositoryId::compatible (itf_id, AbstractBase::repository_id_))
			ab = static_cast <AbstractBase*> (&InterfacePtr (itf));
		else
			throw_INV_OBJREF ();
		InterfacePtr qi = ab->_query_interface (iid);
		if (!qi)
			throw_INV_OBJREF ();
		itf = qi;
	}

	return itf;
}

Object::_ref_type Binder::bind_sync (const Key& name) const
{
	InterfaceRef itf = find (name);
	if (RepositoryId::compatible (itf->_epv ().interface_id, Object::repository_id_))
		return reinterpret_cast <Object::_ref_type&&> (itf);
	else
		throw_INV_OBJREF ();
}

void Binder::bind (ClassLibrary& mod)
{
	SYNC_BEGIN (&singleton_.sync_domain_);
	ModuleContext context { SyncContext::free_sync_context () };
	const ModuleStartup* startup = singleton_.module_bind (mod._get_ptr (), mod.metadata (), &context);
	try {
		if (startup) {
			if (startup->flags & OLF_MODULE_SINGLETON)
				invalid_metadata ();
			ModuleInit::_ptr_type init = ModuleInit::_check (startup->startup);
			mod.initialize (init);
			try {
				singleton_.map_.merge (context.exports);
			} catch (...) {
				mod.terminate ();
				throw;
			}
		}
	} catch (...) {
		module_unbind (mod._get_ptr (), mod.metadata ());
		throw;
	}
	SYNC_END ();
}

void Binder::bind (Singleton& mod)
{
	SYNC_BEGIN (&singleton_.sync_domain_);
	ModuleContext context{ mod.sync_domain () };
	const ModuleStartup* startup = singleton_.module_bind (mod._get_ptr (), mod.metadata (), &context);
	try {
		if (startup) {
			// We don't check for singleton flag here. We allow load usual class library as a singleton.
			//if (startup->flags & OLF_MODULE_SINGLETON)
			//	invalid_metadata ();
			ModuleInit::_ptr_type init = ModuleInit::_check (startup->startup);
			mod.initialize (init);
			try {
				singleton_.map_.merge (context.exports);
			} catch (...) {
				mod.terminate ();
				throw;
			}
		}
	} catch (...) {
		module_unbind (mod._get_ptr (), mod.metadata ());
		throw;
	}
	SYNC_END ();
}

Legacy::Main::_ptr_type Binder::bind (Legacy::Core::Executable& mod)
{
	SYNC_BEGIN (&singleton_.sync_domain_);
	const ModuleStartup* startup = singleton_.module_bind (mod._get_ptr (), mod.metadata (), nullptr);
	try {
		if (!startup || !startup->startup)
			invalid_metadata ();
		return Legacy::Main::_check (startup->startup);
	} catch (...) {
		singleton_.module_unbind (mod._get_ptr (), mod.metadata ());
		throw;
	}
	SYNC_END ();
}

}
}
