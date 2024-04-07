/// \file
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
#ifndef NIRVANA_CORE_BINDER_H_
#define NIRVANA_CORE_BINDER_H_
#pragma once

#include "SyncDomain.h"
#include "Section.h"
#include "Synchronized.h"
#include "Module.h"
#include "BinderMemory.h"
#include <CORBA/RepId.h>
#include <Nirvana/Main.h>
#include <Nirvana/ModuleInit.h>
#include <Nirvana/Domains.h>
#include "WaitableRef.h"
#include "Chrono.h"
#include "MapOrderedUnstable.h"
#include "MapUnorderedStable.h"
#include "ORB/RemoteReferences.h"
#include "TimerAsyncCall.h"

namespace Nirvana {
namespace Core {

class ClassLibrary;
class Singleton;
class Executable;

/// Implements object binding and module loading.
class Binder
{
	/// To avoid priority inversion, if module loader deadline is too far,
	/// it will be temporary adjusted. TODO: config.h
	static const TimeBase::TimeT MODULE_LOADING_DEADLINE_MAX = 1 * TimeBase::SECOND;

public:
	typedef CORBA::Internal::Interface::_ref_type InterfaceRef;
	template <class T>
	using Allocator = BinderMemory::Allocator <T>;

	Binder () :
		housekeeping_domains_on_ (false)
	{}

	~Binder ()
	{}

	static void initialize ();
	static void terminate ();

	// Implements System::bind()
	static CORBA::Object::_ref_type bind (const IDL::String& name)
	{
		CORBA::Object::_ref_type ret;
		SYNC_BEGIN (sync_domain (), nullptr);
		ret = singleton_->bind_sync (name);
		SYNC_END ();
		return ret;
	}

	/// Implements System::bind_interface()
	static InterfaceRef bind_interface (CORBA::Internal::String_in name, CORBA::Internal::String_in iid)
	{
		InterfaceRef ret;
		SYNC_BEGIN (sync_domain (), nullptr);
		ret = singleton_->bind_interface_sync (name, iid);
		SYNC_END ();
		return ret;
	}

	template <class I>
	static CORBA::Internal::I_ref <I> bind_interface (CORBA::Internal::String_in name)
	{
		return bind_interface (name, CORBA::Internal::RepIdOf <I>::id).template downcast <I> ();
	}

	/// Bind executable.
	/// 
	/// \param mod Module interface.
	/// \param metadata Module OLF metadata section.
	/// \returns The Main interface pointer.
	inline static Main::_ptr_type bind (Executable& mod);

	/// Unbind executable.
	/// 
	/// \param mod The Nirvana::Module interface.
	/// \param metadata Module metadata.
	inline static void unbind (Executable& mod) noexcept;

	/// Unmarshal remote reference.
	/// 
	/// \tparam DomainKey Domain key type: ESIOP::ProtDomainId or const IIOP::ListenPoint&.
	/// \param domain Object endpoint.
	/// \param iid Primary interface id.
	/// \param addr IOR address.
	/// \param object_key Domain-relative object key.
	/// \param ORB_type The ORB type.
	/// \param components Tagged components.
	/// \returns CORBA::Object reference.
	template <class EndPoint>
	static CORBA::Object::_ref_type unmarshal_remote_reference (const EndPoint& domain,
		const IDL::String& iid, const IOP::TaggedProfileSeq& addr, const IOP::ObjectKey& object_key,
		uint32_t ORB_type, const IOP::TaggedComponentSeq& components, CORBA::Core::ReferenceRemoteRef& unconfirmed)
	{
		CORBA::Object::_ref_type ret;
		SYNC_BEGIN (sync_domain (), nullptr)
			ret = singleton_->remote_references_.unmarshal (domain, iid, addr, object_key, ORB_type,
				components, unconfirmed);
		SYNC_END ();
		return ret;
	}

	/// Get CORBA::Core::Domain reference.
	/// 
	/// \param domain Domain address.
	/// \returns Reference to CORBA::Core::Domain
	static Ref <CORBA::Core::DomainProt> get_domain (ESIOP::ProtDomainId domain)
	{
		Ref <CORBA::Core::DomainProt> ret;
		SYNC_BEGIN (sync_domain (), nullptr)
			ret = singleton_->remote_references_.get_domain (domain);
		SYNC_END ();
		return ret;
	}

	static Binder& singleton () noexcept
	{
		return singleton_;
	}

	static SyncDomain& sync_domain () noexcept
	{
		return sync_domain_;
	}

	static MemContext& memory () noexcept
	{
		return sync_domain_->mem_context ();
	}

	CORBA::Core::RemoteReferences& remote_references () noexcept
	{
		return remote_references_;
	}

	void erase_reference (const CORBA::OctetSeq& ref, const char* object_name) noexcept
	{
		if (object_name)
			object_map_.erase (object_name);
		remote_references_.erase (ref);
	}

	static bool initialized () noexcept
	{
		return initialized_;
	}

	static void heartbeat (const CORBA::Core::DomainAddress& domain) noexcept
	{
		try {
			Nirvana::Core::Synchronized _sync_frame (sync_domain (), nullptr);
			singleton_->remote_references_.heartbeat (domain);
			_sync_frame.return_to_caller_context ();
		} catch (...) {}
	}

	static void complex_ping (const CORBA::Core::DomainAddress& domain, CORBA::Internal::IORequest::_ptr_type rq)
	{
		SYNC_BEGIN (sync_domain (), nullptr)
			singleton_->remote_references_.complex_ping (domain, rq);
		_sync_frame.return_to_caller_context ();
		SYNC_END ();
	}

	static void clear_remote_references () noexcept
	{
		try {
			Nirvana::Core::Synchronized _sync_frame (sync_domain (), nullptr);
			singleton_->remote_references_.shutdown ();
		} catch (...) {}
	}

	void start_domains_housekeeping ()
	{
		// Called from sync context
		assert (&SyncContext::current () == &sync_domain ());
		if (!housekeeping_domains_on_) {
			const TimeBase::TimeT interval = CORBA::Core::DomainProt::HEARTBEAT_TIMEOUT;
			housekeeping_timer_domains_.set (0, interval, interval);
			housekeeping_domains_on_ = true;
		}
	}

	static CORBA::Object::_ref_type load_and_bind (int32_t mod_id, CORBA::Internal::String_in mod_path,
		CosNaming::NamingContextExt::_ptr_type name_service, bool singleton,
		CORBA::Internal::String_in name)
	{
		CORBA::Object::_ref_type ret;
		SYNC_BEGIN (sync_domain (), nullptr);
		ret = singleton_->load_and_bind_sync (mod_id, mod_path, name_service, singleton,
			ObjectKey (name.data(), name.size ()));
		SYNC_END ();
		return ret;
	}

private:
	typedef CORBA::Internal::RepId RepId;
	typedef CORBA::Internal::RepId::Version Version;
	typedef CORBA::Internal::Interface::_ptr_type InterfacePtr;

	// ObjectKey class for object name and version.
	// Notice that ObjectKey does not store name and works like string view.
	class ObjectKey
	{
	public:
		ObjectKey (const char* id, size_t length) noexcept :
			name_ (id)
		{
			const char* sver = RepId::version (id, id + length);
			length_ = sver - id;
			version_ = Version (sver);
		}

		template <size_t cc>
		ObjectKey (char const (&ar) [cc]) noexcept :
			ObjectKey (ar, cc - 1)
		{}

		ObjectKey (const char* id) noexcept :
			ObjectKey (id, strlen (id))
		{}

		template <class A>
		ObjectKey (const std::basic_string <char, std::char_traits <char>, A>& id) noexcept :
			ObjectKey (id.c_str (), id.length ())
		{}

		ObjectKey (const CORBA::Internal::StringView <char>& id) noexcept :
			ObjectKey (id.c_str (), id.length ())
		{}

		bool operator < (const ObjectKey& rhs) const noexcept
		{
			if (length_ < rhs.length_)
				return true;
			else if (length_ > rhs.length_)
				return false;
			else {
				int c = RepId::lex_compare (name_, name_ + length_, rhs.name_, rhs.name_ + length_);
				if (c < 0)
					return true;
				else if (c > 0)
					return false;
			}
			return version_ < rhs.version_;
		}

		bool name_eq (const ObjectKey& rhs) const noexcept
		{
			return length_ == rhs.length_ && std::equal (name_, name_ + length_, rhs.name_);
		}

		bool compatible (const ObjectKey& rhs) const noexcept
		{
			return name_eq (rhs) && version_.compatible (rhs.version_);
		}

		const char* name () const noexcept
		{
			return name_;
		}

		size_t name_len () const noexcept
		{
			return length_;
		}

	private:
		const char* name_;
		size_t length_;
		Version version_;
	};

	// We use fast binary tree without the iterator stability.
	typedef MapOrderedUnstable <ObjectKey, InterfacePtr, std::less <ObjectKey>,
		Allocator> ObjectMapBase;

	// Name->interface map.
	class ObjectMap : public ObjectMapBase
	{
	public:
		void insert (const char* name, InterfacePtr itf);
		void erase (const char* name) noexcept;
		void merge (const ObjectMap& src);
		InterfacePtr find (const ObjectKey& key) const;
	};

	// Map of the loaded modules.
	// Map must provide the pointer stability.
	// phmap::node_hash_map is declared as with pointer stability,
	// but it seems that not.
	typedef WaitableRef <Module*> ModulePtr;
	typedef MapUnorderedStable
		<int32_t, ModulePtr, std::hash <int32_t>,
		std::equal_to <int32_t>, Allocator> ModuleMap;

	/// Module binding context
	struct ModuleContext
	{
		/// Synchronization context.
		///   If module is ClassLibrary, sync_context is free context.
		///   If module is Singleton, sync_context is the singleton synchronization domain.
		SyncContext& sync_context;

		/// Module exports.
		ObjectMap exports;
	};

	/// Bind module.
	/// 
	/// \param mod The Nirvana::Module interface.
	/// \param metadata Module metadata.
	/// \param mod_context Module binding context.
	///   If module is Executable, mod_context must be `nullptr`.
	/// 
	/// \returns Pointer to the ModuleStartup metadata structure, if found. Otherwise `nullptr`.
	const ModuleStartup* module_bind (Nirvana::Module::_ptr_type mod, const Section& metadata, ModuleContext* mod_context);
	static void module_unbind (Nirvana::Module::_ptr_type mod, const Section& metadata) noexcept;
	inline void remove_exports (const Section& metadata) noexcept;
	static void release_imports (Nirvana::Module::_ptr_type mod, const Section& metadata);

	InterfaceRef bind_interface_sync (const ObjectKey& name, CORBA::Internal::String_in iid);
	CORBA::Object::_ref_type bind_sync (const ObjectKey& name);

	InterfaceRef find (const ObjectKey& name);

	NIRVANA_NORETURN static void invalid_metadata ();

	Ref <Module> load (int32_t mod_id, AccessDirect::_ref_type binary, const IDL::String& mod_path,
		CosNaming::NamingContextExt::_ptr_type name_service, bool singleton);
	void unload (Module* pmod) noexcept;

	CORBA::Object::_ref_type load_and_bind_sync (int32_t mod_id, CORBA::Internal::String_in mod_path,
		CosNaming::NamingContextExt::_ptr_type name_service, bool singleton, const ObjectKey& name)
	{
		load (mod_id, nullptr, mod_path, name_service, singleton);
		InterfaceRef itf = object_map_.find (name);
		if (itf && CORBA::Internal::RepId::compatible (itf->_epv ().interface_id,
			CORBA::Internal::RepIdOf <CORBA::Object>::id))
				return itf.template downcast <CORBA::Object> ();
		throw_OBJECT_NOT_EXIST ();
	}

	void housekeeping_modules ();
	static void delete_module (Module* mod) noexcept;
	void unload_modules ();

	class HousekeepingTimerModules : public TimerAsyncCall
	{
	protected:
		HousekeepingTimerModules () :
			TimerAsyncCall (sync_domain (), INFINITE_DEADLINE)
		{}

	private:
		virtual void run (const TimeBase::TimeT& signal_time);
	};

	void housekeeping_domains () noexcept;

	class HousekeepingTimerDomains : public TimerAsyncCall
	{
	protected:
		HousekeepingTimerDomains () :
			TimerAsyncCall (sync_domain (), INFINITE_DEADLINE)
		{}

	private:
		virtual void run (const TimeBase::TimeT& signal_time);
	};

private:
	Packages::_ref_type packages_;
	CosNaming::NamingContextExt::_ref_type name_service_;
	ObjectMap object_map_;
	ModuleMap module_map_;
	CORBA::Core::RemoteReferences remote_references_;
	ImplStatic <HousekeepingTimerModules> housekeeping_timer_modules_;
	ImplStatic <HousekeepingTimerDomains> housekeeping_timer_domains_;
	bool housekeeping_domains_on_;

	static StaticallyAllocated <ImplStatic <SyncDomainCore> > sync_domain_;
	static StaticallyAllocated <Binder> singleton_;
	static bool initialized_;
};

}
}

#endif
