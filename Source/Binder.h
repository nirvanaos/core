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

#include "SyncDomain.h"
#include "Section.h"
#include "Synchronized.h"
#include "Heap.h"
#include <CORBA/RepositoryId.h>
#include <Nirvana/Main.h>
#include <Nirvana/ModuleInit.h>

#pragma push_macro ("verify")
#undef verify
#include "parallel-hashmap/parallel_hashmap/btree.h"
#pragma pop_macro ("verify")

namespace Nirvana {

namespace Legacy {
namespace Core {
class Executable;
}
}

namespace Core {

class ClassLibrary;
class Singleton;
class Module;

/// Implementation of the interface Nirvana::Binder.
class Binder
{
private:
	typedef CORBA::Internal::RepositoryId RepositoryId;
	typedef CORBA::Internal::RepositoryId::Version Version;
	typedef CORBA::Internal::Interface::_ptr_type InterfacePtr;
	typedef CORBA::Internal::Interface::_ref_type InterfaceRef;

	/// Key class for object name and version.
	/// Notice that Key does not store name and works like string view.
	class Key
	{
	public:
		Key (const char* id, size_t length) :
			name_ (id)
		{
			const char* sver = RepositoryId::version (id, id + length);
			length_ = sver - id;
			version_ = Version (sver);
		}

		Key (const char* id) :
			Key (id, strlen (id))
		{}

		template <class A>
		Key (const std::basic_string <char, std::char_traits <char>, A>& id) :
			Key (id.c_str (), id.length ())
		{}

		bool operator < (const Key& rhs) const
		{
			if (length_ < rhs.length_)
				return true;
			else if (length_ > rhs.length_)
				return false;
			else {
				int c = RepositoryId::lex_compare (name_, name_ + length_, rhs.name_, rhs.name_ + length_);
				if (c < 0)
					return true;
				else if (c > 0)
					return false;
			}
			return version_ < rhs.version_;
		}

		bool is_a (const Key& rhs) const
		{
			return length_ == rhs.length_ && std::equal (name_, name_ + length_, rhs.name_);
		}

		bool compatible (const Key& rhs) const
		{
			return length_ == rhs.length_
				&& !RepositoryId::lex_compare (name_, name_ + length_, rhs.name_, rhs.name_ + length_)
				&& version_.compatible (rhs.version_);
		}

	private:
		const char* name_;
		size_t length_;
		Version version_;
	};

	/// We use phmap::btree_map as fast binary tree without the iterator stability.
	typedef phmap::btree_map <Key, InterfacePtr, std::less <Key>, UserAllocator <std::pair <Key, InterfacePtr> > > MapBase;

	/// Name->interface map.
	class Map : public MapBase
	{
	public:
		void insert (const char* name, InterfacePtr itf);
		void erase (const char* name) NIRVANA_NOEXCEPT;
		void merge (const Map& src);
		InterfacePtr find (const Key& key) const;
	};

public:
	static void initialize ();
	static void terminate ();

	/// Implements System::Bind
	static CORBA::Object::_ref_type bind (CORBA::Internal::String_in name)
	{
		const std::string& sname = static_cast <const std::string&> (name);
		CoreString name_copy (sname.c_str (), sname.length ());
		SYNC_BEGIN (&singleton_.sync_domain_);
		return singleton_.bind_sync (name_copy);
		SYNC_END ();
	}

	/// Implements System::BindInterface
	static InterfaceRef bind_interface (CORBA::Internal::String_in name, CORBA::Internal::String_in iid)
	{
		const std::string& sname = static_cast <const std::string&> (name);
		const std::string& siid = static_cast <const std::string&> (iid);
		CoreString name_copy (sname.c_str (), sname.length ()), iid_copy (siid.c_str (), siid.length ());
		SYNC_BEGIN (&singleton_.sync_domain_);
		return singleton_.bind_interface_sync (name_copy, iid_copy);
		SYNC_END ();
	}

	template <class I>
	static CORBA::Internal::I_ref <I> bind_interface (CORBA::Internal::String_in name)
	{
		return bind_interface (name, I::repository_id_).template downcast <I> ();
	}

	/// Bind ClassLibrary.
	/// 
	/// \param mod ClassLibrary object.
	static void bind (ClassLibrary& mod);

	/// Bind Singleton.
	/// 
	/// \param mod Singleton object.
	static void bind (Singleton& mod);

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

	/// Unbind module.
	/// 
	/// \param mod Module.
	static void unbind (Module& mod) NIRVANA_NOEXCEPT;

private:
	/// Module binding context
	struct ModuleContext
	{
		/// Synchronization context.
		///   If module is ClassLibrary, sync_context is free context.
		///   If module is Singleton, sync_context is the singleton synchronization domain.
		SyncContext& sync_context;

		/// Module exports.
		Map exports;
	};

	/// Bind module.
	/// 
	/// \param mod The Nirvana::Module interface.
	/// \param metadata Module metadata.
	/// \param mod_context Module binding context.
	///   If module is Legacy::Executable, mod_context must be `nullptr`.
	/// 
	/// \returns Pointer to the ModuleStartup metadata structure, if found. Otherwise `nullptr`.
	const ModuleStartup* module_bind (Nirvana::Module::_ptr_type mod, const Section& metadata, ModuleContext* mod_context) const;
	void remove_module_exports (const Section& metadata);
	static void module_unbind (Nirvana::Module::_ptr_type mod, const Section& metadata) NIRVANA_NOEXCEPT;

	InterfaceRef bind_interface_sync (const Key& name, CORBA::Internal::String_in iid) const;
	CORBA::Object::_ref_type bind_sync (const Key& name) const;

	InterfaceRef find (const Key& name) const;

	NIRVANA_NORETURN static void invalid_metadata ();

private:
	ImplStatic <SyncDomain> sync_domain_;
	Map map_;

	static Binder singleton_;
};

}
}

#endif
