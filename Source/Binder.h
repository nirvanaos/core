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
#include "HeapUser.h"
#include "Module.h"
#include "StaticallyAllocated.h"
#include <CORBA/RepId.h>
#include <Nirvana/Main.h>
#include <Nirvana/ModuleInit.h>
#include "WaitableRef.h"
#include "ORB/Services.h"
#include "Chrono.h"

#define BINDER_USE_STD_MODULE_MAP

#if UINTPTR_MAX > UINT32_MAX
#define BINDER_USE_SEPARATE_MEMORY
#endif

#pragma push_macro ("verify")
#undef verify
#include "parallel-hashmap/parallel_hashmap/btree.h"
#ifndef BINDER_USE_STD_MODULE_MAP
#include "parallel-hashmap/parallel_hashmap/phmap.h"
#endif
#pragma pop_macro ("verify")

#ifdef BINDER_USE_STD_MODULE_MAP
#include <unordered_map>
#endif

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
class Binder : private CORBA::Core::Services
{
	/// To avoid priority inversion, if module loader deadline is too far,
	/// it will be temporary adjusted. TODO: config.h
	static const TimeBase::TimeT MODULE_LOADING_DEADLINE_MAX = 1 * TimeBase::SECOND;

public:
	typedef CORBA::Internal::Interface::_ref_type InterfaceRef;
	typedef CORBA::Object::_ref_type ObjectRef;

	Binder () :
		sync_domain_ (std::ref (static_cast <SyncContextCore&> (g_core_free_sync_context)),
			std::ref (static_cast <MemContextCore&> (memory ())))
	{}

	static void initialize ();
	static void terminate ();

	// Implements System::Bind
	static ObjectRef bind (CORBA::Internal::String_in name)
	{
		const std::string& sname = static_cast <const std::string&> (name);
		SharedString name_copy (sname.c_str (), sname.length ());
		SYNC_BEGIN (singleton_->sync_domain_, nullptr);
		return singleton_->bind_sync (name_copy);
		SYNC_END ();
	}

	/// Implements System::BindInterface
	static InterfaceRef bind_interface (CORBA::Internal::String_in name, CORBA::Internal::String_in iid)
	{
		const std::string& sname = static_cast <const std::string&> (name);
		const std::string& siid = static_cast <const std::string&> (iid);
		SharedString name_copy (sname.c_str (), sname.length ()), iid_copy (siid.c_str (), siid.length ());
		SYNC_BEGIN (singleton_->sync_domain_, nullptr);
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
	static Legacy::Main::_ptr_type bind (Legacy::Core::Executable& mod);

	/// Unbind legacy executable.
	/// 
	/// \param mod The Nirvana::Module interface.
	/// \param metadata Module metadata.
	static void unbind (Legacy::Core::Executable& mod) NIRVANA_NOEXCEPT;

	/// Bind service.
	/// 
	/// \param id Service identifier.
	/// \returns Service object reference.
	static ObjectRef bind_service (CORBA::Internal::String_in id);

	/// Bind service.
	/// 
	/// \param sidx Service index.
	/// \returns Service object reference.
	static ObjectRef bind_service (Service sidx);

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

	private:
		const char* name_;
		size_t length_;
		Version version_;
	};

#ifdef BINDER_USE_SEPARATE_MEMORY
	template <class T>
	class Allocator : public std::allocator <T>
	{
	public:
		DEFINE_ALLOCATOR (Allocator);

		static void deallocate (T* p, size_t cnt)
		{
			memory_->heap ().release (p, cnt * sizeof (T));
		}

		static T* allocate (size_t cnt)
		{
			size_t cb = cnt * sizeof (T);
			return (T*)memory_->heap ().allocate (nullptr, cb, 0);
		}
	};
#else
	template <class T>
	using Allocator = SharedAllocator <T>;
#endif

	// We use phmap::btree_map as fast binary tree without the iterator stability.
	typedef phmap::btree_map <ObjectKey, InterfacePtr, std::less <ObjectKey>,
		Allocator <std::pair <ObjectKey, InterfacePtr> > > ObjectMapBase;

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
	typedef 
#ifndef BINDER_USE_STD_MODULE_MAP
		phmap::node_hash_map
#else
		std::unordered_map
#endif
		<std::string, ModulePtr, phmap::Hash <std::string>,
		phmap::EqualTo <std::string>, Allocator <std::pair <std::string, ModulePtr> >
	> ModuleMap;

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
	void remove_exports (const Section& metadata) NIRVANA_NOEXCEPT;
	static void release_imports (Nirvana::Module::_ptr_type mod, const Section& metadata);

	InterfaceRef bind_interface_sync (const ObjectKey& name, CORBA::Internal::String_in iid);
	ObjectRef bind_sync (const ObjectKey& name);

	InterfaceRef find (const ObjectKey& name);

	NIRVANA_NORETURN static void invalid_metadata ();

	CoreRef <Module> load (std::string& module_name, bool singleton);
	void unload (Module* pmod);

	void housekeeping ();

	void delete_module (Module* mod);

	void unload_modules ();

	static MemContext& memory () NIRVANA_NOEXCEPT
	{
#ifdef BINDER_USE_SEPARATE_MEMORY
		return memory_;
#else
		return g_shared_mem_context;
#endif
	}

private:
#ifdef BINDER_USE_SEPARATE_MEMORY
	static StaticallyAllocated <ImplStatic <MemContextCore> > memory_;
#endif
	ImplStatic <SyncDomainImpl> sync_domain_;
	ObjectMap object_map_;
	ModuleMap module_map_;

	static StaticallyAllocated <Binder> singleton_;
	static bool initialized_;

};

}
}

#endif
