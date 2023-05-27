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
#include <Nirvana/Legacy/Main.h>
#include <Nirvana/ModuleInit.h>
#include "WaitableRef.h"
#include "Chrono.h"
#include "MapOrderedUnstable.h"
#include "MapUnorderedStable.h"
#include "ORB/RemoteReferences.h"
#include "TimerAsyncCall.h"

namespace Nirvana {

namespace Legacy {
namespace Core {
class Executable;
}
}

namespace Core {

class ClassLibrary;
class Singleton;

/// Implements object binding and module loading.
class Binder
{
	/// To avoid priority inversion, if module loader deadline is too far,
	/// it will be temporary adjusted. TODO: config.h
	static const TimeBase::TimeT MODULE_LOADING_DEADLINE_MAX = 1 * TimeBase::SECOND;

public:
	typedef CORBA::Internal::Interface::_ref_type InterfaceRef;
	typedef CORBA::Object::_ref_type ObjectRef;
	template <class T>
	using Allocator = BinderMemory::Allocator <T>;

	Binder ()
	{}

	~Binder ()
	{}

	static void initialize ();
	static void terminate ();

	// Implements System::Bind
	static ObjectRef bind (CORBA::Internal::String_in name)
	{
		const std::string& sname = static_cast <const std::string&> (name);
		SharedString name_copy (sname.c_str (), sname.length ());
		SYNC_BEGIN (sync_domain (), nullptr);
		return singleton_->bind_sync (name_copy);
		SYNC_END ();
	}

	/// Implements System::BindInterface
	static InterfaceRef bind_interface (CORBA::Internal::String_in name, CORBA::Internal::String_in iid)
	{
		const std::string& sname = static_cast <const std::string&> (name);
		const std::string& siid = static_cast <const std::string&> (iid);
		SharedString name_copy (sname.c_str (), sname.length ()), iid_copy (siid.c_str (), siid.length ());
		SYNC_BEGIN (sync_domain (), nullptr);
		return singleton_->bind_interface_sync (name_copy, iid_copy);
		SYNC_END ();
	}

	template <class I>
	static CORBA::Internal::I_ref <I> bind_interface (CORBA::Internal::String_in name)
	{
		return bind_interface (name, CORBA::Internal::RepIdOf <I>::id).template downcast <I> ();
	}

	/// Bind legacy process executable.
	/// 
	/// \param mod Module interface.
	/// \param metadata Module OLF metadata section.
	/// \returns The Main interface pointer.
	inline static Legacy::Main::_ptr_type bind (Legacy::Core::Executable& mod);

	/// Unbind legacy executable.
	/// 
	/// \param mod The Nirvana::Module interface.
	/// \param metadata Module metadata.
	inline static void unbind (Legacy::Core::Executable& mod) NIRVANA_NOEXCEPT;

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
		SYNC_BEGIN (sync_domain (), nullptr)
			return singleton_->remote_references_.unmarshal (domain, iid, addr, object_key, ORB_type,
				components, unconfirmed);
		SYNC_END ()
	}

	/// Get CORBA::Core::Domain reference.
	/// 
	/// \param domain Domain address.
	/// \returns Reference to CORBA::Core::Domain
	static CORBA::servant_reference <ESIOP::DomainProt> get_domain (ESIOP::ProtDomainId domain)
	{
		SYNC_BEGIN (sync_domain (), nullptr)
			return singleton_->remote_references_.get_domain (domain);
		SYNC_END ();
	}

	static Binder& singleton () NIRVANA_NOEXCEPT
	{
		return singleton_;
	}

	static SyncDomain& sync_domain () NIRVANA_NOEXCEPT
	{
		return sync_domain_;
	}

	static MemContext& memory () NIRVANA_NOEXCEPT
	{
		return sync_domain_->mem_context ();
	}

	CORBA::Core::RemoteReferences& remote_references () NIRVANA_NOEXCEPT
	{
		return remote_references_;
	}

	void erase_reference (const CORBA::OctetSeq& ref, const char* object_name) NIRVANA_NOEXCEPT
	{
		if (object_name)
			object_map_.erase (object_name);
		remote_references_.erase (ref);
	}

	static bool initialized () NIRVANA_NOEXCEPT
	{
		return initialized_;
	}

	static void heartbeat (const CORBA::Core::DomainAddress& domain) NIRVANA_NOEXCEPT
	{
		try {
			Nirvana::Core::Synchronized _sync_frame (sync_domain (), nullptr);
			singleton_->remote_references_.heartbeat (domain);
			_sync_frame.return_to_caller_context ();
		} catch (...) {}
	}

	static void complex_ping (CORBA::Core::RequestIn& rq)
	{
		SYNC_BEGIN (sync_domain (), nullptr)
			singleton_->remote_references_.complex_ping (rq);
		_sync_frame.return_to_caller_context ();
		SYNC_END ();
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
		ObjectKey (const char* id, size_t length) NIRVANA_NOEXCEPT :
			name_ (id)
		{
			const char* sver = RepId::version (id, id + length);
			length_ = sver - id;
			version_ = Version (sver);
		}

		template <size_t cc>
		ObjectKey (char const (&ar) [cc]) NIRVANA_NOEXCEPT :
			ObjectKey (ar, cc - 1)
		{}

		ObjectKey (const char* id) NIRVANA_NOEXCEPT :
			ObjectKey (id, strlen (id))
		{}

		template <class A>
		ObjectKey (const std::basic_string <char, std::char_traits <char>, A>& id) NIRVANA_NOEXCEPT :
			ObjectKey (id.c_str (), id.length ())
		{}

		bool operator < (const ObjectKey& rhs) const NIRVANA_NOEXCEPT
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

		bool operator == (const ObjectKey& rhs) const NIRVANA_NOEXCEPT
		{
			return length_ == rhs.length_ && std::equal (name_, name_ + length_, rhs.name_);
		}

		bool compatible (const ObjectKey& rhs) const NIRVANA_NOEXCEPT
		{
			return operator == (rhs) && version_.compatible (rhs.version_);
		}

		const char* name () const NIRVANA_NOEXCEPT
		{
			return name_;
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
		void erase (const char* name) NIRVANA_NOEXCEPT;
		void merge (const ObjectMap& src);
		InterfacePtr find (const ObjectKey& key) const;
	};

	// Map of the loaded modules.
	// Map must provide the pointer stability.
	// phmap::node_hash_map is declared as with pointer stability,
	// but it seems that not.
	typedef WaitableRef <Module*> ModulePtr;
	typedef MapUnorderedStable
		<std::string, ModulePtr, std::hash <std::string>,
		std::equal_to <std::string>, Allocator> ModuleMap;

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
	///   If module is Legacy::Executable, mod_context must be `nullptr`.
	/// 
	/// \returns Pointer to the ModuleStartup metadata structure, if found. Otherwise `nullptr`.
	const ModuleStartup* module_bind (Nirvana::Module::_ptr_type mod, const Section& metadata, ModuleContext* mod_context);
	static void module_unbind (Nirvana::Module::_ptr_type mod, const Section& metadata) NIRVANA_NOEXCEPT;
	inline void remove_exports (const Section& metadata) NIRVANA_NOEXCEPT;
	static void release_imports (Nirvana::Module::_ptr_type mod, const Section& metadata);

	InterfaceRef bind_interface_sync (const ObjectKey& name, CORBA::Internal::String_in iid);
	ObjectRef bind_sync (const ObjectKey& name);

	InterfaceRef find (const ObjectKey& name);

	NIRVANA_NORETURN static void invalid_metadata ();

	Ref <Module> load (std::string& module_name, bool singleton);
	void unload (Module* pmod);

	void housekeeping ();

	void delete_module (Module* mod);

	inline void unload_modules ();

	class HousekeepingTimer : public TimerAsyncCall
	{
	protected:
		HousekeepingTimer () :
			TimerAsyncCall (sync_domain (), INFINITE_DEADLINE)
		{}

	private:
		virtual void run (const TimeBase::TimeT& signal_time)
		{
			singleton ().housekeeping ();
		}
	};

private:
	static StaticallyAllocated <ImplStatic <SyncDomainCore> > sync_domain_;
	ObjectMap object_map_;
	ModuleMap module_map_;
	CORBA::Core::RemoteReferences remote_references_;
	ImplStatic <HousekeepingTimer> housekeeping_timer_;

	static StaticallyAllocated <Binder> singleton_;
	static bool initialized_;
};

}
}

#endif
