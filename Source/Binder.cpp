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
#include "Nirvana/Domains.h"

namespace Nirvana {

namespace Legacy {
class Factory;
extern const ImportInterfaceT <Factory> g_factory;
}

namespace Core {

using namespace CORBA;
using namespace CORBA::Internal;
using namespace CORBA::Core;

#ifndef BINDER_USE_SHARED_MEMORY
StaticallyAllocated <ImplStatic <MemContextCore> > Binder::memory_;
#endif
StaticallyAllocated <ImplStatic <SyncDomainCore> > Binder::sync_domain_;
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
#ifndef BINDER_USE_SHARED_MEMORY
	memory_.construct ();
#endif
	sync_domain_.construct (std::ref (static_cast <MemContextCore&> (memory ())));
	singleton_.construct ();
	Section metadata;
	if (!Port::SystemInfo::get_OLF_section (metadata))
		throw_INITIALIZE ();

	SYNC_BEGIN (sync_domain (), nullptr);
	ModuleContext context{ g_core_free_sync_context };
	singleton_->module_bind (nullptr, metadata, &context);
	singleton_->object_map_ = std::move (context.exports);
	initialized_ = true;
	SYNC_END ();
}

inline
void Binder::unload_modules ()
{
	bool unloaded;
	do {
		// Unload unbound modules
		unloaded = false;
		for (auto it = module_map_.begin (); it != module_map_.end ();) {
			Module* pmod = it->second.get_if_constructed ();
			// if pmod == nullptr, then module loading error was occurred.
			// Module was not loaded.
			// Just remove the entry from table.
			if (!pmod || !pmod->bound ()) {
				it = module_map_.erase (it);
				if (pmod)
					unload (pmod);
				unloaded = true;
			} else
				++it;
		}
	} while (unloaded);

	// If everything is OK, molule map must be empty
	assert (module_map_.empty ());

	// If so, unload the bound modules also.
	while (!module_map_.empty ()) {
		auto it = module_map_.begin ();
		Module* pmod = it->second.get ();
		module_map_.erase (it);
		unload (pmod);
	}
}

void Binder::terminate ()
{
	if (!initialized_)
		return;
	SYNC_BEGIN (g_core_free_sync_context, &memory ());

	SYNC_BEGIN (sync_domain (), nullptr);
	initialized_ = false;
	singleton_->unload_modules ();
	SYNC_END ();
	Section metadata;
	Port::SystemInfo::get_OLF_section (metadata);
	ExecDomain& ed = ExecDomain::current ();
	ed.restricted_mode (ExecDomain::RestrictedMode::SUPPRESS_ASYNC_GC);
	singleton_->module_unbind (nullptr, metadata);
	
	SYNC_BEGIN (sync_domain (), nullptr);
	singleton_.destruct ();
	SYNC_END ();

	sync_domain_.destruct ();
#ifndef BINDER_USE_SHARED_MEMORY
	memory_.destruct ();
#endif

	SYNC_END ();
}

NIRVANA_NORETURN void Binder::invalid_metadata ()
{
	throw std::runtime_error ("Invalid file format");
}

const ModuleStartup* Binder::module_bind (::Nirvana::Module::_ptr_type mod, const Section& metadata, ModuleContext* mod_context)
{
	TLS& tls = TLS::current ();
	void* prev_context = tls.get (TLS::CORE_TLS_BINDER);
	tls.set (TLS::CORE_TLS_BINDER, mod_context);

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
		ObjectKey k_legacy (Legacy::g_factory.imp.name);
		for (OLF_Iterator it (metadata.address, metadata.size); !it.end (); it.next ()) {
			switch (*it.cur ()) {
				case OLF_IMPORT_INTERFACE:
					if (!module_entry) {
						ImportInterface* ps = reinterpret_cast <ImportInterface*> (it.cur ());
						assert (!ps->itf);
						ObjectKey key (ps->name);
						if (key == k_gmodule) {
							assert (mod);
							if (!k_gmodule.compatible (key))
								invalid_metadata ();
							module_entry = ps;
							break;
						} else if (mod_context) {
							if (key == k_legacy)
								invalid_metadata (); // Only legacy process can import Legacy::Factory interface
						} else if (key == k_object_factory)
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
					const_cast <void*> (metadata.address), const_cast <size_t&> (metadata.size),
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
							PortableServer::Core::ServantBase* core_object = PortableServer::Core::ServantBase::create (
									Type <PortableServer::Servant>::in (ps->servant_base));
							Object::_ptr_type obj = core_object->proxy ().get_proxy ();
							ps->core_object = &PortableServer::Servant (core_object);
							mod_context->exports.insert (ps->name, obj);
						} break;

						case OLF_EXPORT_LOCAL: {
							ExportLocal* ps = reinterpret_cast <ExportLocal*> (it.cur ());
							CORBA::Core::LocalObject* core_object = CORBA::Core::LocalObject::create (
									Type <CORBA::LocalObject>::in (ps->local_object));
							Object::_ptr_type obj = core_object->proxy ().get_proxy ();
							ps->core_object = &CORBA::LocalObject::_ptr_type (core_object);
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
						const CORBA::Internal::StringView <char> requested_iid (ps->interface_id);
						if (RepId::compatible (obj->_epv ().header.interface_id, requested_iid))
							reinterpret_cast <Object::_ref_type&> (ps->itf) = std::move (obj);
						else {
							InterfacePtr itf = obj->_query_interface (requested_iid);
							if (!itf)
								throw_INV_OBJREF ();
							ps->itf = interface_duplicate (&itf);
						}
					}
				}

			if (Port::Memory::FLAGS & Memory::ACCESS_CHECK)
				verify (Port::Memory::copy (const_cast <void*> (metadata.address),
					const_cast <void*> (metadata.address), const_cast <size_t&> (metadata.size),
					Memory::READ_ONLY | Memory::EXACTLY));
		}
	} catch (...) {
		SYNC_BEGIN (mod_context->sync_context, nullptr);
		module_unbind (mod, { metadata.address, metadata.size });
		SYNC_END ();
		tls.set (TLS::CORE_TLS_BINDER, prev_context);
		throw;
	}

	tls.set (TLS::CORE_TLS_BINDER, prev_context);

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
		SYNC_BEGIN (g_core_free_sync_context, &memory ());
		delete mod;
		SYNC_END ();
	}
}

Ref <Module> Binder::load (std::string& module_name, bool singleton)
{
	if (!initialized_)
		throw_INITIALIZE ();
	Module* mod = nullptr;
	auto ins = module_map_.emplace (std::move (module_name), MODULE_LOADING_DEADLINE_MAX);
	ModuleMap::reference entry = *ins.first;
	if (ins.second) {
		try {
			SYNC_BEGIN (g_core_free_sync_context, &memory ());
			if (singleton)
				mod = new Singleton (entry.first);
			else
				mod = new ClassLibrary (entry.first);
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
			entry.second.on_exception ();
			delete_module (mod);
			module_map_.erase (entry.first);
			throw;
		}

		entry.second.finish_construction (mod);

	} else {
		mod = entry.second.get ();
		if (mod->singleton () != singleton)
			throw_BAD_PARAM ();
	}
	return mod;
}

void Binder::unload (Module* pmod)
{
	remove_exports (pmod->metadata ());
	SYNC_BEGIN (pmod->sync_context (), pmod);
	pmod->terminate ();
	module_unbind (pmod->_get_ptr (), pmod->metadata ());
	SYNC_END ();
	delete_module (pmod);
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
	ExecDomain& ed = ExecDomain::current ();
	ExecDomain::RestrictedMode rm = ed.restricted_mode ();
	ed.restricted_mode (ExecDomain::RestrictedMode::SUPPRESS_ASYNC_GC);
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
	ed.restricted_mode (rm);
}

Binder::InterfaceRef Binder::find (const ObjectKey& name)
{
	const ExecDomain& exec_domain = ExecDomain::current ();
	if (ExecDomain::RestrictedMode::MODULE_TERMINATE == exec_domain.restricted_mode ())
		throw_NO_PERMISSION ();
	ModuleContext* context = reinterpret_cast <ModuleContext*> (TLS::current ().get (TLS::CORE_TLS_BINDER));
	InterfaceRef itf;
	if (context)
		itf = context->exports.find (name);
	if (!itf) {
		if (!initialized_)
			throw_OBJECT_NOT_EXIST ();
		itf = object_map_.find (name);
		if (!itf) {
			SysDomain::_ref_type sys_domain = SysDomain::_narrow (Services::bind (Services::SysDomain));
			BindInfo bind_info;
			sys_domain->get_bind_info (name.name (), 0, bind_info);
			if (bind_info._d ()) {
				Ref <Module> mod = load (bind_info.module_name (), false);
				itf = object_map_.find (name);
				if (!itf)
					throw_OBJECT_NOT_EXIST ();
			} else {
				CORBA::Object::_ptr_type obj = bind_info.obj ();
				CORBA::Core::ReferenceRemote* ref = static_cast <CORBA::Core::ReferenceRemote*> (
					CORBA::Core::ProxyManager::cast (obj));
				if (!ref)
					throw_OBJECT_NOT_EXIST ();
				try {
					object_map_.insert (ref->set_object_name (name.name ()), &obj);
				} catch (...) {
					assert (false);
				}
				return std::move (bind_info.obj ());
			}
		}
	}
	return itf;
}

Binder::InterfaceRef Binder::bind_interface_sync (const ObjectKey& name, String_in iid)
{
	InterfaceRef itf = find (name);
	CORBA::Internal::StringView <char> itf_id = itf->_epv ().interface_id;
	if (!RepId::compatible (itf_id, iid)) {
		if (!RepId::compatible (itf_id, RepIdOf <PseudoBase>::id))
			throw_INV_OBJREF ();
		PseudoBase::_ptr_type b = static_cast <PseudoBase*> (&InterfacePtr (itf));
		InterfacePtr qi = b->_query_interface (iid);
		if (!qi)
			throw_INV_OBJREF ();
		itf = qi;
	}

	return itf;
}

Object::_ref_type Binder::bind_sync (const ObjectKey& name)
{
	InterfaceRef itf = find (name);
	if (RepId::compatible (itf->_epv ().interface_id, RepIdOf <Object>::id))
		return reinterpret_cast <Object::_ref_type&&> (itf);
	else
		throw_INV_OBJREF ();
}

void Binder::housekeeping ()
{
	for (;;) {
		bool found = false;
		SteadyTime t = Chrono::steady_clock ();
		if (1 <= MODULE_UNLOAD_TIMEOUT)
			break;
		t -= MODULE_UNLOAD_TIMEOUT;
		for (auto it = module_map_.begin (); it != module_map_.end ();) {
			Module* pmod = it->second.get_if_constructed ();
			if (pmod && pmod->can_be_unloaded (t)) {
				found = true;
				it = module_map_.erase (it);
				unload (pmod);
				break;
			} else
				++it;
		}
		if (!found)
			break;
	}
}

}
}
