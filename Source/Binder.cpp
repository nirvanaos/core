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
#include "pch.h"
#include <CORBA/Server.h>
#include "Binder.inl"
#include <Port/SystemInfo.h>
#include <Nirvana/OLF.h>
#include <Nirvana/OLF_Iterator.h>
#include "ORB/ServantBase.h"
#include "ORB/LocalObject.h"
#include "ClassLibrary.h"
#include "Singleton.h"
#include "Executable.h"
#include "Packages.h"
#include "Nirvana/Domains.h"

using namespace CORBA;
using namespace CORBA::Internal;
using namespace CORBA::Core;

namespace Nirvana {
namespace Core {

StaticallyAllocated <ImplStatic <SyncDomainCore> > Binder::sync_domain_;
StaticallyAllocated <Binder> Binder::singleton_;
bool Binder::initialized_ = false;

void Binder::ObjectMap::insert (const char* name, InterfacePtr itf)
{
	assert (itf);
	assert (*name);
	if (!ObjectMapBase::emplace (name, itf).second)
		throw_INV_OBJREF ();	// Duplicated name TODO: Log
}

void Binder::ObjectMap::erase (const char* name) noexcept
{
	NIRVANA_VERIFY (ObjectMapBase::erase (name));
}

void Binder::ObjectMap::merge (const ObjectMap& src)
{
	for (auto it = src.begin (); it != src.end (); ++it) {
		if (!ObjectMapBase::insert (*it).second) {
			for (auto it1 = src.begin (); it1 != it; ++it1) {
				ObjectMapBase::erase (it1->first);
			}
			throw_INV_OBJREF ();	// Duplicated name TODO: Log
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
	BinderMemory::initialize ();
	sync_domain_.construct (std::ref (BinderMemory::heap ()));
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
	housekeeping_timer_modules_.cancel ();

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
				NIRVANA_TRACE ("Unload module %d", it->first);
				it = module_map_.erase (it);
				if (pmod)
					unload (pmod);
				unloaded = true;
			} else
				++it;
		}
	} while (unloaded);

	// If everything is OK, molule map must be empty
	NIRVANA_ASSERT_EX (module_map_.empty (), true);

	// If so, unload the bound modules also.
	while (!module_map_.empty ()) {
		auto it = module_map_.begin ();
		NIRVANA_WARNING ("Force unload module %d", it->first);
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
	singleton_->packages_ = nullptr;
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

	SYNC_END ();

	BinderMemory::terminate ();
}

NIRVANA_NORETURN void Binder::invalid_metadata ()
{
	throw_INTERNAL (make_minor_errno (ENOEXEC));
}

const ModuleStartup* Binder::module_bind (::Nirvana::Module::_ptr_type mod, const Section& metadata, ModuleContext* mod_context)
{
	ExecDomain& ed = ExecDomain::current ();
	void* prev_context = ed.TLS_get (CoreTLS::CORE_TLS_BINDER);
	ed.TLS_set (CoreTLS::CORE_TLS_BINDER, mod_context);

	enum MetadataFlags
	{
		IMPORT_INTERFACES = 0x01,
		EXPORT_OBJECTS = 0x02,
		IMPORT_OBJECTS = 0x04
	};

	ImportInterface* module_entry = nullptr;
	const ModuleStartup* module_startup = nullptr;

	try {

		// Pass 1: Export pseudo objects and bind module.
		unsigned flags = 0;
		ObjectKey k_gmodule (the_module.imp.name);
		ObjectKey k_object_factory (object_factory.imp.name);
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
						} else if (!mod_context && key == k_object_factory)
							invalid_metadata (); // Process can not import ObjectFactory interface
					}
					flags |= MetadataFlags::IMPORT_INTERFACES;
					break;

				case OLF_EXPORT_INTERFACE: {
					if (!mod_context // Process can not export
						|| // Singleton can not export interfaces
						mod_context->sync_context.sync_context_type () == SyncContext::Type::SYNC_DOMAIN_SINGLETON
						)
						invalid_metadata ();

					const ExportInterface* ps = reinterpret_cast <const ExportInterface*> (it.cur ());
					mod_context->exports.insert (ps->name, ps->itf);
				} break;

				case OLF_EXPORT_OBJECT:
				case OLF_EXPORT_LOCAL:
					if (!mod_context) // Process can not export
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
				NIRVANA_VERIFY (Port::Memory::copy (const_cast <void*> (metadata.address),
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
				assert (mod_context); // Executable can not export.
				SYNC_BEGIN (mod_context->sync_context, nullptr);
				for (OLF_Iterator it (metadata.address, metadata.size); !it.end (); it.next ()) {
					switch (*it.cur ()) {
						case OLF_EXPORT_OBJECT: {
							ExportObject* ps = reinterpret_cast <ExportObject*> (it.cur ());
							PortableServer::Core::ServantBase* core_object = PortableServer::Core::ServantBase::create (
									Type <PortableServer::Servant>::in (ps->servant_base), nullptr);
							Object::_ptr_type obj = core_object->proxy ().get_proxy ();
							ps->core_object = &PortableServer::Servant (core_object);
							mod_context->exports.insert (ps->name, obj);
						} break;

						case OLF_EXPORT_LOCAL: {
							ExportLocal* ps = reinterpret_cast <ExportLocal*> (it.cur ());
							CORBA::Core::LocalObject* core_object = CORBA::Core::LocalObject::create (
									Type <CORBA::LocalObject>::in (ps->local_object), nullptr);
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
				NIRVANA_VERIFY (Port::Memory::copy (const_cast <void*> (metadata.address),
					const_cast <void*> (metadata.address), const_cast <size_t&> (metadata.size),
					Memory::READ_ONLY | Memory::EXACTLY));
		}
	} catch (...) {
		if (mod_context) {
			SYNC_BEGIN (mod_context->sync_context, nullptr);
			module_unbind (mod, { metadata.address, metadata.size });
			SYNC_END ();
		} else
			module_unbind (mod, { metadata.address, metadata.size });
		ed.TLS_set (CoreTLS::CORE_TLS_BINDER, prev_context);
		throw;
	}

	ed.TLS_set (CoreTLS::CORE_TLS_BINDER, prev_context);

	return module_startup;
}

void Binder::module_unbind (Nirvana::Module::_ptr_type mod, const Section& metadata) noexcept
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

void Binder::delete_module (Module* mod) noexcept
{
	if (mod) {
		try {
			// Module deletion can be a long operation, do it in system context
			SYNC_BEGIN (g_core_free_sync_context, &memory ());
			Module* tmp = mod;
			mod = nullptr;
			delete tmp;
			SYNC_END ();
		} catch (...) {
			if (mod) {
				ExecDomain& ed = ExecDomain::current ();
				Ref <MemContext> tmp (&memory ());
				ed.mem_context_swap (tmp);
				delete mod;
				ed.mem_context_swap (tmp);
			}
		}
	}
}

Ref <Module> Binder::load (int32_t mod_id, AccessDirect::_ref_type binary,
	const IDL::String& mod_path, CosNaming::NamingContextExt::_ref_type name_service,
	bool singleton)
{
	if (!initialized_)
		throw_INITIALIZE ();
	Module* mod = nullptr;
	auto ins = module_map_.emplace (mod_id, MODULE_LOADING_DEADLINE_MAX);
	ModuleMap::reference entry = *ins.first;
	if (ins.second) {
		auto wait_list = entry.second.wait_list ();
		try {

			// Module loading may be a long operation, do it in core context.
			SYNC_BEGIN (g_core_free_sync_context, &memory ());
			
			if (!binary) {
				
				if (!name_service)
					name_service = CosNaming::NamingContextExt::_narrow (CORBA::Core::Services::bind (
						CORBA::Core::Services::NameService));

				binary = Packages::open_binary (name_service, mod_path);
			}

			if (singleton)
				mod = new Singleton (mod_id, binary);
			else
				mod = new ClassLibrary (mod_id, binary);

			SYNC_END ();

			assert (mod->_refcount_value () == 0);
			ModuleContext context { mod->sync_context () };
			const ModuleStartup* startup = module_bind (mod->_get_ptr (), mod->metadata (), &context);
			try {

				// Module without an entry point is a special case reserved for future.
				// Currently we prohibit it.
				if (!startup)
					invalid_metadata ();

				if (startup && (startup->flags & OLF_MODULE_SINGLETON) && !singleton)
					invalid_metadata ();

				SYNC_BEGIN (context.sync_context, mod->initterm_mem_context ());
				mod->initialize (startup ? ModuleInit::_check (startup->startup) : nullptr);
				SYNC_END ();

				mod->initial_ref_cnt_ = mod->_refcount_value ();

				try {
					object_map_.merge (context.exports);
				} catch (...) {
					SYNC_BEGIN (context.sync_context, mod->initterm_mem_context ());
					mod->terminate ();
					SYNC_END ();
					throw;
				}

			} catch (...) {
				SYNC_BEGIN (context.sync_context, mod->initterm_mem_context ());
				module_unbind (mod->_get_ptr (), mod->metadata ());
				SYNC_END ();
				throw;
			}
		} catch (...) {
			wait_list->on_exception ();
			delete_module (mod);
			module_map_.erase (entry.first);
			throw;
		}

		wait_list->finish_construction (mod);
		if (module_map_.size () == 1)
			housekeeping_timer_modules_.set (0, MODULE_UNLOAD_TIMEOUT, MODULE_UNLOAD_TIMEOUT);

	} else {
		mod = entry.second.get ();
		if (mod->singleton () != singleton)
			throw_BAD_PARAM ();
	}
	return mod;
}

void Binder::unload (Module* mod) noexcept
{
	remove_exports (mod->metadata ());

	{
		::Nirvana::Core::Synchronized _sync_frame (mod->sync_context (), mod->initterm_mem_context ());

		mod->execute_atexit ();
		mod->terminate ();
		module_unbind (mod->_get_ptr (), mod->metadata ());

		NIRVANA_ASSERT_EX (mod->_refcount_value () == 1, true);
	}
	
	delete_module (mod);
}

inline
void Binder::remove_exports (const Section& metadata) noexcept
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
	switch (exec_domain.sync_context ().sync_context_type ()) {
	case SyncContext::Type::FREE_MODULE_TERM:
	case SyncContext::Type::SINGLETON_TERM:
		throw_NO_PERMISSION ();
	}
	ModuleContext* context = reinterpret_cast <ModuleContext*> (exec_domain.TLS_get (CoreTLS::CORE_TLS_BINDER));
	InterfaceRef itf;
	if (context)
		itf = context->exports.find (name);
	if (!itf) {
		if (!initialized_)
			throw_OBJECT_NOT_EXIST ();
		itf = object_map_.find (name);
		if (!itf) {
			if (!packages_)
				packages_ = SysDomain::_narrow (Services::bind (Services::SysDomain))->provide_packages ();
			BindInfo bind_info;
			packages_->get_bind_info (name.name (), PLATFORM, bind_info);
			if (bind_info._d ()) {
				try {
					const ModuleLoad& ml = bind_info.module_load ();
					load (ml.module_id (), ml.binary (), IDL::String (), nullptr, false);
				} catch (const SystemException&) {
					// TODO: Log
					throw_OBJECT_NOT_EXIST ();
				}
				itf = object_map_.find (name);
				if (!itf)
					throw_OBJECT_NOT_EXIST ();
			} else {
				Object::_ptr_type obj = bind_info.loaded_object ();
				ProxyManager* proxy = ProxyManager::cast (obj);
				if (!proxy)
					throw_OBJECT_NOT_EXIST ();
				Reference* ref = proxy->to_reference ();
				if (ref && !(ref->flags () & Reference::LOCAL)) {
					ReferenceRemote& rr = static_cast <ReferenceRemote&> (*ref);
					object_map_.insert (rr.set_object_name (name.name ()), &obj);
				}
#ifndef NDEBUG
				else
					assert (object_map_.find (name));
#endif
				return std::move (bind_info.loaded_object ());
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
		InterfacePtr qi = nullptr;
		if (RepId::compatible (itf_id, RepIdOf <PseudoBase>::id)) {
			PseudoBase::_ptr_type b = static_cast <PseudoBase*> (&InterfacePtr (itf));
			qi = b->_query_interface (iid);
		} else if (RepId::compatible (itf_id, RepIdOf <ValueBase>::id)) {
			ValueBase::_ptr_type b = static_cast <ValueBase*> (&InterfacePtr (itf));
			qi = b->_query_valuetype (iid);
		}
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
		return itf.template downcast <CORBA::Object> ();
	else
		throw_INV_OBJREF ();
}

inline
void Binder::housekeeping_modules ()
{
	for (;;) {
		bool found = false;
		SteadyTime t = Chrono::steady_clock ();
		if (t <= MODULE_UNLOAD_TIMEOUT)
			break;
		t -= MODULE_UNLOAD_TIMEOUT;
		for (auto it = module_map_.begin (); it != module_map_.end ();) {
			Module* pmod = it->second.get_if_constructed ();
			if (pmod && pmod->can_be_unloaded (t)) {
				found = true;
				it = module_map_.erase (it);
				unload (pmod); // Causes context switch
				break;
			} else
				++it;
		}
		if (!found)
			break;
	}
	if (module_map_.empty ())
		housekeeping_timer_modules_.cancel ();
}

void Binder::HousekeepingTimerModules::run (const TimeBase::TimeT& signal_time)
{
	singleton ().housekeeping_modules ();
}

inline
void Binder::housekeeping_domains () noexcept
{
	if (!remote_references_.housekeeping ()) {
		housekeeping_timer_domains_.cancel ();
		housekeeping_domains_on_ = false;
	}
}

void Binder::HousekeepingTimerDomains::run (const TimeBase::TimeT& signal_time)
{
	singleton ().housekeeping_domains ();
}

}
}
