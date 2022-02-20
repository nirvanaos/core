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
#include "Binder.inl"
#include <Port/SystemInfo.h>
#include <Nirvana/OLF.h>
#include <Nirvana/OLF_Iterator.h>
#include "ORB/ServantBase.h"
#include "ORB/LocalObject.h"
#include "ClassLibrary.h"
#include "Singleton.h"
#include "Legacy/Executable.h"

namespace Nirvana {
namespace Core {

using namespace std;
using namespace CORBA;
using namespace CORBA::Internal;

StaticallyAllocated <Binder> Binder::singleton_;
bool Binder::initialized_ = false;

void Binder::ObjectMap::insert (const char* name, InterfacePtr itf)
{
	assert (itf);
	if (!ObjectMapBase::emplace (name, itf).second)
		throw_INV_OBJREF ();	// Duplicated name
}

void Binder::ObjectMap::erase (const char* name) NIRVANA_NOEXCEPT
{
	verify (ObjectMapBase::erase (name));
}

void Binder::ObjectMap::merge (const ObjectMap& src)
{
	for (auto it = src.begin (); it != src.end (); ++it) {
		if (!ObjectMapBase::insert (*it).second) {
			for (auto it1 = src.begin (); it1 != it; ++it1) {
				ObjectMapBase::erase (it1->first);
			}
			throw_INV_OBJREF ();	// Duplicated name
		}
	}
}

Binder::InterfacePtr Binder::ObjectMap::find (const ObjectKey& key) const
{
	auto pf = lower_bound (key);
	if (pf != end () && pf->first.compatible (key)) {
		return pf->second;
	}
	return nullptr;
}

void Binder::initialize ()
{
	singleton_.construct ();
	Section metadata;
	if (!Port::SystemInfo::get_OLF_section (metadata))
		throw_INITIALIZE ();

	SYNC_BEGIN (singleton_->sync_domain_, nullptr);
	ModuleContext context{ g_core_free_sync_context };
	singleton_->module_bind (nullptr, metadata, &context);
	singleton_->object_map_ = std::move (context.exports);
	initialized_ = true;
	SYNC_END ();
}

void Binder::terminate ()
{
	SYNC_BEGIN (singleton_->sync_domain_, nullptr);
	assert (initialized_);
	initialized_ = false;
	while (!singleton_->module_map_.empty ())
		singleton_->unload (singleton_->module_map_.begin ());
	SYNC_END ();
	singleton_.destruct ();
}

NIRVANA_NORETURN void Binder::invalid_metadata ()
{
	throw runtime_error ("Invalid file format");
}

const ModuleStartup* Binder::module_bind (::Nirvana::Module::_ptr_type mod, const Section& metadata, ModuleContext* mod_context)
{
	ExecDomain& exec_domain = ExecDomain::current ();
	void* prev_context = exec_domain.binder_context_;
	exec_domain.binder_context_ = mod_context;

	enum MetadataFlags
	{
		IMPORT_INTERFACES = 0x01,
		EXPORT_OBJECTS = 0x02,
		IMPORT_OBJECTS = 0x04
	};

	ImportInterface* module_entry = nullptr;
	const ModuleStartup* module_startup = nullptr;

	try {

		// Pass 1: Export pseudo objects and bind g_module.
		unsigned flags = 0;
		ObjectKey k_gmodule (g_module.imp.name);
		ObjectKey k_object_factory (g_object_factory.imp.name);
		for (OLF_Iterator it (metadata.address, metadata.size); !it.end (); it.next ()) {
			switch (*it.cur ()) {
				case OLF_IMPORT_INTERFACE:
					if (!module_entry) {
						ImportInterface* ps = reinterpret_cast <ImportInterface*> (it.cur ());
						ObjectKey key (ps->name);
						if (key == k_gmodule) {
							assert (mod);
							if (!k_gmodule.compatible (key))
								invalid_metadata ();
							module_entry = ps;
							break;
						} else if (!mod_context && key == k_object_factory)
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

		if (flags || module_entry) {
			if (Port::Memory::FLAGS & Memory::ACCESS_CHECK)
				verify (Port::Memory::copy (const_cast <void*> (metadata.address),
					const_cast <void*> (metadata.address), metadata.size,
					Memory::READ_WRITE | Memory::EXACTLY));

			if (module_entry)
				module_entry->itf = &mod;

			// Pass 2: Import pseudo objects.
			if (flags & MetadataFlags::IMPORT_INTERFACES)
				for (OLF_Iterator it (metadata.address, metadata.size); !it.end (); it.next ()) {
					if (OLF_IMPORT_INTERFACE == *it.cur ()) {
						ImportInterface* ps = reinterpret_cast <ImportInterface*> (it.cur ());
						if (!mod || ps != module_entry)
							reinterpret_cast <InterfaceRef&> (ps->itf) = bind_interface_sync (
								ps->name, ps->interface_id);
					}
				}

			// Pass 3: Export objects.
			if (flags & MetadataFlags::EXPORT_OBJECTS) {
				assert (mod_context); // Legacy executable can not export.
				SYNC_BEGIN (mod_context->sync_context, nullptr);
				for (OLF_Iterator it (metadata.address, metadata.size); !it.end (); it.next ()) {
					switch (*it.cur ()) {
						case OLF_EXPORT_OBJECT: {
							ExportObject* ps = reinterpret_cast <ExportObject*> (it.cur ());
							PortableServer::Servant core_obj;
							core_obj = (
								new PortableServer::Core::ServantBase (
									Type <PortableServer::Servant>::in (ps->servant_base)
								))->_get_ptr ();
							Object::_ptr_type obj = AbstractBase::_ptr_type (core_obj)->_query_interface <Object> ();
							ps->core_object = &core_obj;
							mod_context->exports.insert (ps->name, obj);
						} break;

						case OLF_EXPORT_LOCAL: {
							ExportLocal* ps = reinterpret_cast <ExportLocal*> (it.cur ());
							LocalObject::_ptr_type core_obj;
							core_obj = (
								new CORBA::Internal::Core::LocalObject (Type <LocalObject>::in (ps->local_object),
									Type <AbstractBase>::in (ps->abstract_base)))->_get_ptr ();
							Object::_ptr_type obj = core_obj;
							ps->core_object = &core_obj;
							mod_context->exports.insert (ps->name, obj);
						} break;
					}
				}
				SYNC_END ();
			}

			// Pass 4: Import objects.
			if (flags & MetadataFlags::IMPORT_OBJECTS)
				for (OLF_Iterator it (metadata.address, metadata.size); !it.end (); it.next ()) {
					if (OLF_IMPORT_OBJECT == *it.cur ()) {
						ImportInterface* ps = reinterpret_cast <ImportInterface*> (it.cur ());
						Object::_ref_type obj = bind_sync (ps->name);
						const StringBase <char> requested_iid (ps->interface_id);
						if (RepId::compatible (obj->_epv ().header.interface_id, requested_iid))
							reinterpret_cast <Object::_ref_type&> (ps->itf) = move (obj);
						else {
							InterfacePtr itf = AbstractBase::_ptr_type (obj)->_query_interface (requested_iid);
							if (!itf)
								throw_INV_OBJREF ();
							ps->itf = interface_duplicate (&itf);
						}
					}
				}

			if (Port::Memory::FLAGS & Memory::ACCESS_CHECK)
				verify (Port::Memory::copy (const_cast <void*> (metadata.address),
					const_cast <void*> (metadata.address), metadata.size,
					Memory::READ_ONLY | Memory::EXACTLY));
		}
	} catch (...) {
		SYNC_BEGIN (mod_context->sync_context, nullptr);
		module_unbind (mod, { metadata.address, metadata.size });
		SYNC_END ();
		exec_domain.binder_context_ = prev_context;
		throw;
	}

	exec_domain.binder_context_ = prev_context;

	return module_startup;
}

void Binder::module_unbind (Nirvana::Module::_ptr_type mod, const Section& metadata) NIRVANA_NOEXCEPT
{
	// Pass 1: Release all imported interfaces.
	release_imports (mod, metadata);

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

void Binder::delete_module (Module* mod)
{
	if (mod) {
		SYNC_BEGIN (g_core_free_sync_context, nullptr);
		delete mod;
		SYNC_END ();
	}
}

CoreRef <Module> Binder::load (string& module_name, bool singleton)
{
	if (!initialized_)
		throw_INITIALIZE ();
	Module* mod = nullptr;
	auto ins = module_map_.emplace (move (module_name), MODULE_LOADING_DEADLINE_MIN);
	if (ins.second) {
		try {

			SYNC_BEGIN (g_core_free_sync_context, &sync_domain_.mem_context ());
			if (singleton)
				mod = new Singleton (ins.first->first);
			else
				mod = new ClassLibrary (ins.first->first);
			SYNC_END ();
			
			assert (mod->_refcount_value () == 0);
			ModuleContext context{ mod->sync_context () };
			const ModuleStartup* startup = module_bind (mod->_get_ptr (), mod->metadata (), &context);
			try {
				if (startup && (startup->flags & OLF_MODULE_SINGLETON) && !singleton)
					invalid_metadata ();

				auto initial_ref_cnt = mod->_refcount_value ();
				SYNC_BEGIN (context.sync_context, mod);
				mod->initialize (startup ? ModuleInit::_check (startup->startup) : nullptr, initial_ref_cnt);
				SYNC_END ();

				try {
					object_map_.merge (context.exports);
				} catch (...) {
					SYNC_BEGIN (context.sync_context, mod);
					mod->terminate ();
					SYNC_END ();
					throw;
				}

			} catch (...) {
				SYNC_BEGIN (context.sync_context, mod);
				module_unbind (mod->_get_ptr (), mod->metadata ());
				SYNC_END ();
				throw;
			}
		} catch (...) {
			ins.first->second.on_exception ();
			delete_module (mod);
			module_map_.erase (ins.first);
			throw;
		}

		ins.first->second.finish_construction (mod);

	} else {
		mod = ins.first->second.get ();
		if (mod->singleton () != singleton)
			throw_BAD_PARAM ();
	}
	return mod;
}

void Binder::unload (ModuleMap::iterator mod)
{
	Module* pmod = nullptr;
	try {
		pmod = mod->second.get ();
		assert (!pmod->bound ());
	} catch (...) {
		// Module loading error was occurred.
		// Module was not loaded.
		// Just remove the entry from table.
	}
	module_map_.erase (mod);
	if (pmod) {
		remove_exports (pmod->metadata ());
		SYNC_BEGIN (pmod->sync_context (), pmod);
		pmod->terminate ();
		module_unbind (pmod->_get_ptr (), pmod->metadata ());
		SYNC_END ();
		delete_module (pmod);
	}
}

inline
void Binder::remove_exports (const Section& metadata) NIRVANA_NOEXCEPT
{
	// Pass 1: Remove all exports from the map.
	// This pass will not cause inter-domain calls.
	for (OLF_Iterator it (metadata.address, metadata.size); !it.end (); it.next ()) {
		switch (*it.cur ()) {
			case OLF_EXPORT_INTERFACE: {
				const ExportInterface* ps = reinterpret_cast <const ExportInterface*> (it.cur ());
				object_map_.erase (ps->name);
			} break;

			case OLF_EXPORT_OBJECT: {
				ExportObject* ps = reinterpret_cast <ExportObject*> (it.cur ());
				object_map_.erase (ps->name);
			} break;

			case OLF_EXPORT_LOCAL: {
				ExportLocal* ps = reinterpret_cast <ExportLocal*> (it.cur ());
				object_map_.erase (ps->name);
			} break;
		}
	}
}

void Binder::release_imports (Nirvana::Module::_ptr_type mod, const Section& metadata)
{
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
}

Binder::InterfaceRef Binder::find (const ObjectKey& name)
{
	const ExecDomain& exec_domain = ExecDomain::current ();
	if (ExecDomain::RestrictedMode::MODULE_TERMINATE == exec_domain.restricted_mode_)
		throw_NO_PERMISSION ();
	ModuleContext* context = reinterpret_cast <ModuleContext*> (exec_domain.binder_context_);
	InterfaceRef itf;
	if (context)
		itf = context->exports.find (name);
	if (!itf) {
		if (!initialized_)
			throw_OBJECT_NOT_EXIST ();
		itf = object_map_.find (name);
		if (!itf) {
			string module_name = "TestModule.olf";
			CoreRef <Module> mod = load (module_name, false);
			itf = object_map_.find (name);
			if (!itf)
				throw_OBJECT_NOT_EXIST ();
		}
	}
	return itf;
}

Binder::InterfaceRef Binder::bind_interface_sync (const ObjectKey& name, String_in iid)
{
	InterfaceRef itf = find (name);
	StringBase <char> itf_id = itf->_epv ().interface_id;
	if (!RepId::compatible (itf_id, iid)) {
		if (!RepId::compatible (itf_id, AbstractBase::repository_id_))
			throw_INV_OBJREF ();
		AbstractBase::_ptr_type ab = static_cast <AbstractBase*> (&InterfacePtr (itf));
		InterfacePtr qi = ab->_query_interface (iid);
		if (!qi)
			throw_INV_OBJREF ();
		itf = qi;
	}

	return itf;
}

Object::_ref_type Binder::bind_sync (const ObjectKey& name)
{
	InterfaceRef itf = find (name);
	if (RepId::compatible (itf->_epv ().interface_id, Object::repository_id_))
		return reinterpret_cast <Object::_ref_type&&> (itf);
	else
		throw_INV_OBJREF ();
}

void Binder::housekeeping ()
{
	for (;;) {
		bool found = false;
		Chrono::Duration t = Chrono::steady_clock () - MODULE_UNLOAD_TIMEOUT;
		for (auto it = module_map_.begin (); it != module_map_.end (); ++it) {
			Module* pmod = it->second.get_if_constructed ();
			if (pmod && pmod->can_be_unloaded (t)) {
				found = true;
				unload (it);
				break;
			}
		}
		if (!found)
			break;
	}
}

CORBA::Object::_ref_type Binder::bind_service (CORBA::Internal::String_in id)
{
	return bind_service (find_service (static_cast <const string&> (id).c_str ()));
}

CORBA::Object::_ref_type Binder::bind_service (Service sidx)
{
	if ((size_t)sidx >= Service::SERVICE_COUNT)
		throw ORB::InvalidName ();
	SYNC_BEGIN (singleton_->sync_domain_, nullptr);
	return singleton_->bind_service_sync (sidx);
	SYNC_END ();
}

}
}
