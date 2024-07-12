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
#include <CORBA/Proxy/ProxyBase.h>
#include <Nirvana/Domains.h>
#include <Nirvana/OLF_Iterator.h>
#include "Binder.inl"
#include <Port/SystemInfo.h>
#include "ORB/ServantBase.h"
#include "ORB/LocalObject.h"
#include "ORB/RequestLocalBase.h"
#include "ClassLibrary.h"
#include "Singleton.h"
#include "Executable.h"
#include "packagedb.h"

using namespace CORBA;
using namespace CORBA::Internal;
using namespace CORBA::Core;

namespace Nirvana {
namespace Core {

class Binder::Request : public RequestLocalBase
{
public:
	static Ref <Request> create ()
	{
		return Ref <Request>::create <RequestLocalImpl <Request> > ();
	}

protected:
	Request () : RequestLocalBase (&BinderMemory::heap (),
		IORequest::RESPONSE_EXPECTED | IORequest::RESPONSE_DATA)
	{}
};

StaticallyAllocated <ImplStatic <SyncDomainCore> > Binder::sync_domain_;
StaticallyAllocated <Binder> Binder::singleton_;
bool Binder::initialized_ = false;

void Binder::ObjectMap::insert (const char* name, InterfacePtr itf, SyncContext* sc)
{
	assert (itf);
	assert (*name);
	if (!ObjectMapBase::emplace (name, ObjectVal (itf, sc)).second)
		throw_INV_OBJREF ();	// Duplicated name TODO: Log
}

void Binder::ObjectMap::erase (const char* name) noexcept
{
	NIRVANA_VERIFY (ObjectMapBase::erase (name));
}

void Binder::ObjectMap::merge (const ObjectMap& src)
{
	for (auto it = src.begin (); it != src.end (); ++it) {
		auto ins = ObjectMapBase::insert (*it);
		if (!ins.second && ins.first->first.version ().minor < it->first.version ().minor) {
			ObjectMapBase::erase (ins.first);
			ObjectMapBase::insert (*it);
		}
	}
}

const Binder::ObjectVal* Binder::ObjectMap::find (const ObjectKey& key) const
{
	auto pf = ObjectMapBase::find (key);
	if (pf != end () && key.version ().minor <= pf->first.version ().minor)
		return &pf->second;
	return nullptr;
}

void Binder::initialize ()
{
	BinderMemory::initialize ();
	sync_domain_.construct (std::ref (BinderMemory::heap ()));
	singleton_.construct ();
	Section metadata;
	metadata.address = Port::SystemInfo::get_OLF_section (metadata.size);
	if (!metadata.address)
		throw_INITIALIZE ();

	SYNC_BEGIN (sync_domain (), nullptr);
	ModuleContext context (nullptr, g_core_free_sync_context);
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

void Binder::terminate () noexcept
{
	if (!initialized_)
		return;

	try {

		SYNC_BEGIN (g_core_free_sync_context, &memory ());

		SYNC_BEGIN (sync_domain (), nullptr);
		initialized_ = false;
		singleton_->packages_ = nullptr;
		singleton_->unload_modules ();
		SYNC_END ();
		Section metadata;
		metadata.address = Port::SystemInfo::get_OLF_section (metadata.size);
		ExecDomain& ed = ExecDomain::current ();
		ed.restricted_mode (ExecDomain::RestrictedMode::SUPPRESS_ASYNC_GC);
		singleton_->module_unbind (nullptr, metadata);

		SYNC_BEGIN (sync_domain (), nullptr);
		singleton_.destruct ();
		SYNC_END ();

		sync_domain_.destruct ();

		SYNC_END ();

	} catch (...) {
		assert (false);
	}

	BinderMemory::terminate ();
}

const ModuleStartup* Binder::module_bind (::Nirvana::Module::_ptr_type mod,
	const Section& metadata, ModuleContext* mod_context)
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
		for (OLF_Iterator <> it (metadata.address, metadata.size); !it.end (); it.next ()) {
			if (!it.valid ())
				BindError::throw_invalid_metadata ();

			switch (*it.cur ()) {
				case OLF_IMPORT_INTERFACE:
					if (!module_entry) {
						ImportInterface* ps = reinterpret_cast <ImportInterface*> (it.cur ());
						assert (!ps->itf);
						ObjectKey key (ps->name);
						if (key.name_eq (k_gmodule)) {
							assert (mod);
							if (!k_gmodule.compatible (key))
								BindError::throw_message ("Module interface version mismatch");
							module_entry = ps;
							break;
						} else if (!mod_context && key.name_eq (k_object_factory))
							BindError::throw_message ("Process must not import ObjectFactory interface");
					}
					flags |= MetadataFlags::IMPORT_INTERFACES;
					break;

				case OLF_EXPORT_INTERFACE: {
					if (!mod_context) // Process can not export
						BindError::throw_message ("Process must not export interfaces");
					if (mod_context->sync_context.sync_context_type () == SyncContext::Type::SYNC_DOMAIN_SINGLETON)
						BindError::throw_message ("Singleton must not export interfaces");

					const ExportInterface* ps = reinterpret_cast <const ExportInterface*> (it.cur ());
					mod_context->exports.insert (ps->name, ps->itf, &mod_context->sync_context);
				} break;

				case OLF_EXPORT_OBJECT:
				case OLF_EXPORT_LOCAL:
					if (!mod_context) // Process can not export
						BindError::throw_message ("Process must not export interfaces");
					flags |= MetadataFlags::EXPORT_OBJECTS;
					break;

				case OLF_IMPORT_OBJECT:
					flags |= MetadataFlags::IMPORT_OBJECTS;
					break;

				case OLF_MODULE_STARTUP:
					if (module_startup)
						BindError::throw_message ("Duplicated OLF_MODULE_STARTUP entry");
					module_startup = reinterpret_cast <const ModuleStartup*> (it.cur ());
					break;

				default:
					BindError::throw_invalid_metadata ();
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
				for (OLF_Iterator <> it (metadata.address, metadata.size); !it.end (); it.next ()) {
					if (OLF_IMPORT_INTERFACE == *it.cur ()) {
						ImportInterface* ps = reinterpret_cast <ImportInterface*> (it.cur ());
						if (!mod || ps != module_entry)
							reinterpret_cast <InterfaceRef&> (ps->itf) = bind_interface_sync (
								ps->name, ps->interface_id).itf;
					}
				}

			// Pass 3: Export objects.
			if (flags & MetadataFlags::EXPORT_OBJECTS) {
				assert (mod_context); // Executable can not export.
				SYNC_BEGIN (mod_context->sync_context, nullptr);
				for (OLF_Iterator <> it (metadata.address, metadata.size); !it.end (); it.next ()) {
					switch (*it.cur ()) {
						case OLF_EXPORT_OBJECT: {
							ExportObject* ps = reinterpret_cast <ExportObject*> (it.cur ());
							PortableServer::Core::ServantBase* core_object = PortableServer::Core::ServantBase::create (
									Type <PortableServer::Servant>::in (ps->servant), nullptr);
							Object::_ptr_type obj = core_object->proxy ().get_proxy ();
							ps->core_object = &PortableServer::Servant (core_object);
							mod_context->exports.insert (ps->name, obj, &mod_context->sync_context);
						} break;

						case OLF_EXPORT_LOCAL: {
							ExportObject* ps = reinterpret_cast <ExportObject*> (it.cur ());
							CORBA::Core::LocalObject* core_object = CORBA::Core::LocalObject::create (
									Type <CORBA::LocalObject>::in (ps->servant), nullptr);
							Object::_ptr_type obj = core_object->proxy ().get_proxy ();
							ps->core_object = &CORBA::LocalObject::_ptr_type (core_object);
							mod_context->exports.insert (ps->name, obj, &mod_context->sync_context);
						} break;
					}
				}
				SYNC_END ();
			}

			// Pass 4: Import objects.
			if (flags & MetadataFlags::IMPORT_OBJECTS)
				for (OLF_Iterator <> it (metadata.address, metadata.size); !it.end (); it.next ()) {
					if (OLF_IMPORT_OBJECT == *it.cur ()) {
						ImportInterface* ps = reinterpret_cast <ImportInterface*> (it.cur ());
						reinterpret_cast <InterfaceRef&> (ps->itf) = bind_interface_sync (
							ps->name, ps->interface_id).itf;
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
	for (OLF_Iterator <> it (metadata.address, metadata.size); !it.end (); it.next ()) {
		switch (*it.cur ()) {
			case OLF_EXPORT_OBJECT:
			case OLF_EXPORT_LOCAL: {
				ExportObject* ps = reinterpret_cast <ExportObject*> (it.cur ());
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

Ref <Module> Binder::load (int32_t mod_id, AccessDirect::_ptr_type binary)
{
	if (!initialized_)
		throw_INITIALIZE ();

	Module* mod = nullptr;
	auto ins = module_map_.emplace (mod_id, MODULE_LOADING_DEADLINE_MAX);
	ModuleMap::reference entry = *ins.first;
	if (ins.second) {
		try {
			auto wait_list = entry.second.wait_list ();
			try {

				// Module loading may be a long operation, do it in core context.
				SYNC_BEGIN (g_core_free_sync_context, &memory ());
				mod = Module::create (mod_id, binary);
				SYNC_END ();

				assert (mod->_refcount_value () == 0);
				ModuleContext context (mod, mod->sync_context (), mod_id);
				bind_and_init (*mod, context);
				try {
					object_map_.merge (context.exports);
				} catch (...) {
					terminate_and_unbind (*mod);
					throw;
				}

			} catch (...) {
				module_map_.erase (entry.first);
				wait_list->on_exception ();
				delete_module (mod);
				throw;
			}

			wait_list->finish_construction (mod);
			if (module_map_.size () == 1)
				housekeeping_timer_modules_.set (0, MODULE_UNLOAD_TIMEOUT, MODULE_UNLOAD_TIMEOUT);

		} catch (const SystemException& ex) {
			BindError::Error err;
			BindError::set_system (err, ex);
			BindError::push (err).mod_info (BindError::ModInfo (mod_id, PLATFORM));
			throw err;
		} catch (BindError::Error& err) {
			BindError::push (err).mod_info (BindError::ModInfo (mod_id, PLATFORM));
			throw err;
		}
	} else {
		mod = entry.second.get ();
	}
	return mod;
}

void Binder::bind_and_init (Module& mod, ModuleContext& context)
{
	const ModuleStartup* startup = module_bind (mod._get_ptr (), mod.metadata (), &context);

	SYNC_BEGIN (context.sync_context, mod.initterm_mem_context ());

	try {
		// Module without an entry point is a special case reserved for future.
		// Currently we prohibit it.
		if (!startup)
			BindError::throw_message ("Entry point not found");

		mod.initialize (startup ? ModuleInit::_check (startup->startup) : nullptr);

	} catch (const SystemException& ex) {
		module_unbind (mod._get_ptr (), mod.metadata ());
		BindError::Error err;
		BindError::set_system (err, ex);
		BindError::set_message (BindError::push (err), "Module initialization error");
		throw err;
	}

	SYNC_END ();

	mod.initial_ref_cnt_ = mod._refcount_value ();
}

void Binder::unload (Module* mod) noexcept
{
	remove_exports (mod->metadata ());
	terminate_and_unbind (*mod);
	delete_module (mod);
}

void Binder::terminate_and_unbind (Module& mod) noexcept
{
	try {
		Synchronized _sync_frame (mod.sync_context (), mod.initterm_mem_context ());

		mod.execute_atexit ();
		mod.terminate ();
		module_unbind (mod._get_ptr (), mod.metadata ());
		NIRVANA_ASSERT_EX (mod._refcount_value () == 1, true);
	} catch (...) {
		release_imports (mod._get_ptr (), mod.metadata ());
		assert (false);
	}
}

inline
void Binder::remove_exports (const Section& metadata) noexcept
{
	// Pass 1: Remove all exports from the map.
	// This pass will not cause inter-domain calls.
	for (OLF_Iterator <> it (metadata.address, metadata.size); !it.end (); it.next ()) {
		switch (*it.cur ()) {
			case OLF_EXPORT_INTERFACE: {
				const ExportInterface* ps = reinterpret_cast <const ExportInterface*> (it.cur ());
				object_map_.erase (ps->name);
			} break;

			case OLF_EXPORT_OBJECT:
			case OLF_EXPORT_LOCAL: {
				ExportObject* ps = reinterpret_cast <ExportObject*> (it.cur ());
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
	for (OLF_Iterator <> it (metadata.address, metadata.size); !it.end (); it.next ()) {
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

Binder::BindResult Binder::find (const ObjectKey& name)
{
	const ExecDomain& exec_domain = ExecDomain::current ();
	switch (exec_domain.sync_context ().sync_context_type ()) {
	case SyncContext::Type::FREE_MODULE_TERM:
	case SyncContext::Type::SINGLETON_TERM:
		throw_NO_PERMISSION ();
	}
	ModuleContext* context = reinterpret_cast <ModuleContext*> (exec_domain.TLS_get (CoreTLS::CORE_TLS_BINDER));
	BindResult ret;
	if (context)
		ret = context->exports.find (name);
	if (!ret.itf) {
		if (!initialized_)
			throw_OBJECT_NOT_EXIST ();
		ret = object_map_.find (name);
		if (!ret.itf) {

			Binding binding;
			
			SysDomainCore::_narrow (Services::bind (Services::SysDomain))->get_binding (
				CORBA::Internal::StringView <char> (name.name (), name.full_length ()), PLATFORM, binding);

			if (binding._d ()) {
				const ModuleLoad& ml = binding.module_load ();
				try {
					load (ml.module_id (), ml.binary ());
				} catch (BindError::Error& ex) {
					BindError::push_obj_name (ex, name.name ());
					throw ex;
				}
				ret = object_map_.find (name);
				if (!ret.itf)
					throw_object_not_in_module (name, ml.module_id ());
			} else {
				Object::_ptr_type obj = binding.loaded_object ();
				ProxyManager* proxy = ProxyManager::cast (obj);
				if (!proxy)
					throw_OBJECT_NOT_EXIST ();
				Reference* ref = proxy->to_reference ();

				// If object is local, then it is already inserted in the object map.
				// Otherwise insert it.
				if (ref && !(ref->flags () & Reference::LOCAL)) {
					ReferenceRemote& rr = static_cast <ReferenceRemote&> (*ref);
					object_map_.insert (rr.set_object_name (name.name ()), &obj, nullptr);
				}
#ifndef NDEBUG
				else
					assert (object_map_.find (name));
#endif
				ret.itf = std::move (binding.loaded_object ());
			}
		}

		if (context && context->collect_dependencies) {
			bool system = false;
			if (ret.sync_context) {
				Module* pmod = ret.sync_context->module ();
				if (!pmod || pmod->id () <= MAX_SYS_MODULE_ID)
					system = true;
			}
			if (!system)
				context->dependencies.insert (name);
		}
	}
	return ret;
}

NIRVANA_NORETURN void Binder::throw_object_not_in_module (const ObjectKey& name, int32_t mod_id)
{
	BindError::Error ex;
	BindError::set_message (ex.info (), "Object not found in module");
	BindError::push (ex).mod_info (BindError::ModInfo (mod_id, PLATFORM));
	BindError::push_obj_name (ex, name.name ());
	throw ex;
}

Binder::BindResult Binder::bind_interface (CORBA::Internal::String_in name, CORBA::Internal::String_in iid)
{
	BindResult ret;
	Ref <Request> rq = Request::create ();
	rq->invoke ();
	{
		Synchronized _sync_frame (sync_domain (), nullptr);
		try {
			rq->unmarshal_end ();
			ret = singleton_->bind_interface_sync (name, iid);
			rq->success ();
		} catch (CORBA::Exception& e) {
			rq->set_exception (std::move (e));
		} catch (...) {
			rq->set_unknown_exception ();
		}
	}
	CORBA::Internal::ProxyRoot::check_request (rq->_get_ptr ());
	return ret;
}

Binder::BindResult Binder::bind_interface_sync (const ObjectKey& name, String_in iid)
{
	BindResult ret = find (name);
	query_interface (ret, iid, name);
	return ret;
}

Binder::InterfaceRef Binder::query_interface (InterfacePtr itf, CORBA::Internal::String_in iid,
	const ObjectKey& name)
{
	InterfacePtr ret = nullptr;
	if (itf) {
		CORBA::Internal::StringView <char> itf_id = itf->_epv ().interface_id;
		if (RepId::compatible (itf_id, iid))
			ret = itf;
		else {
			if (RepId::compatible (itf_id, RepIdOf <Object>::id)) {
				Object::_ptr_type b = static_cast <Object*> (&itf);
				ret = b->_query_interface (iid);
			} else if (RepId::compatible (itf_id, RepIdOf <PseudoBase>::id)) {
				PseudoBase::_ptr_type b = static_cast <PseudoBase*> (&itf);
				ret = b->_query_interface (iid);
			} else if (RepId::compatible (itf_id, RepIdOf <ValueBase>::id)) {
				ValueBase::_ptr_type b = static_cast <ValueBase*> (&itf);
				ret = b->_query_valuetype (iid);
			}
		}
	}

	if (!ret) {
		BindError::Error err;
		err.info ().s (iid);
		err.info ()._d (BindError::Type::ERR_ITF_NOT_FOUND);
		BindError::Info& info = BindError::push (err);
		info.s (name.name ());
		info._d (BindError::Type::ERR_OBJ_NAME);
		throw err;
	}
		
	return ret;
}

inline
Binder::BindResult Binder::load_and_bind_sync (int32_t mod_id, AccessDirect::_ptr_type binary,
	const ObjectKey& name, String_in iid)
{
	load (mod_id, binary);
	const ObjectVal* ov = object_map_.find (name);
	if (ov && ov->sync_context) {
		Module* mod = ov->sync_context->module ();
		if (mod && mod->id () == mod_id) {
			BindResult ret;
			ret.itf = query_interface (ov->itf, iid, name);
			ret.sync_context = ov->sync_context;
			return ret;
		}
	}
	throw_object_not_in_module (name, mod_id);
}

Binder::BindResult Binder::load_and_bind (int32_t mod_id,
	Nirvana::AccessDirect::_ptr_type binary, String_in name, String_in iid)
{
	Binder::BindResult ret;
	Ref <Request> rq = Request::create ();
	rq->invoke ();
	{
		Synchronized _sync_frame (sync_domain (), nullptr);
		try {
			rq->unmarshal_end ();
			ret = singleton_->load_and_bind_sync (mod_id, binary, ObjectKey (name.data (), name.size ()), iid);
			rq->success ();
		} catch (CORBA::Exception& e) {
			rq->set_exception (std::move (e));
		} catch (...) {
			rq->set_unknown_exception ();
		}
	}
	CORBA::Internal::ProxyRoot::check_request (rq->_get_ptr ());
	return ret;
}

PM::ModuleBindings Binder::get_module_bindings_sync (Nirvana::AccessDirect::_ptr_type binary)
{
	Module* mod = nullptr;
	SYNC_BEGIN (g_core_free_sync_context, &memory ());
	mod = Module::create (-1, binary);
	SYNC_END ();

	ModuleContext context (mod, mod->sync_context (), true);
	try {
		bind_and_init (*mod, context);
	} catch (...) {
		delete_module (mod);
		throw;
	}

	PM::ModuleBindings bindings;

	bindings.flags ((uint16_t)mod->flags ());

	try {
		for (const auto& el : context.exports) {
			uint_fast16_t flags = OBJECT_FLAG_EXPORT;
			InterfacePtr itf = el.second.itf;
			if (RepId::compatible (itf->_epv ().interface_id, RepIdOf <Object>::id)) {
				const ServantProxyBase* proxy = object2proxy_base (itf.template downcast <Object> ());
				InterfacePtr servant = proxy->servant ();
				if (RepId::compatible (servant->_epv ().interface_id, RepIdOf <PortableServer::ServantBase>::id))
					flags |= OBJECT_FLAG_OBJECT;
			}
			bindings.bindings ().emplace_back (IDL::String (el.first.name (), el.first.name_length ()),
				((uint32_t)(el.first.version ().major) << 16) + el.first.version ().minor, flags);
		}
		for (const auto& el : context.dependencies) {
			bindings.bindings ().emplace_back (IDL::String (el.name (), el.name_length ()),
				((uint32_t)(el.version ().major) << 16) + el.version ().minor, 0);
		}
	} catch (...) {
		terminate_and_unbind (*mod);
		delete_module (mod);
		throw;
	}

	terminate_and_unbind (*mod);
	delete_module (mod);

	return bindings;
}

PM::ModuleBindings Binder::get_module_bindings (AccessDirect::_ptr_type binary)
{
	Ref <Request> rq = Request::create ();
	rq->invoke ();
	{
		Synchronized _sync_frame (sync_domain (), nullptr);
		try {
			rq->unmarshal_end ();
			PM::ModuleBindings md = singleton_->get_module_bindings_sync (binary);
			Type <PM::ModuleBindings>::marshal_out (md, rq->_get_ptr ());
			rq->success ();
		} catch (CORBA::Exception& e) {
			rq->set_exception (std::move (e));
		} catch (...) {
			rq->set_unknown_exception ();
		}
	}
	CORBA::Internal::ProxyRoot::check_request (rq->_get_ptr ());
	PM::ModuleBindings ret;
	Type <PM::ModuleBindings>::unmarshal (rq->_get_ptr (), ret);
	rq->unmarshal_end ();
	return ret;
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
