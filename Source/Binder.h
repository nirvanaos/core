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

#include <CORBA/Server.h>
#include <Binder_s.h>
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
namespace Core {

class ClassLibrary;
class Singleton;

class Binder :
	public CORBA::Nirvana::ServantStatic <Binder, ::Nirvana::Binder>
{
private:
	typedef CORBA::Nirvana::RepositoryId RepositoryId;
	typedef CORBA::Nirvana::RepositoryId::Version Version;
	typedef CORBA::Nirvana::Interface::_ptr_type InterfacePtr;
	typedef CORBA::Nirvana::Interface::_ref_type InterfaceRef;

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

		Key (const std::string& id) :
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

	typedef phmap::btree_map <Key, InterfacePtr, std::less <Key>, CoreAllocator <std::pair <Key, InterfacePtr> > > Map;

public:
	static void initialize ();
	static void terminate ();

	static CORBA::Object::_ref_type bind (const std::string& _name)
	{
		CoreString name (_name.c_str (), _name.length ());
		SYNC_BEGIN (&singleton_.sync_domain_);
		return singleton_.bind_sync (name);
		SYNC_END ();
	}

	static InterfaceRef bind_pseudo (const std::string& _name, const std::string& _iid)
	{
		CoreString name (_name.c_str (), _name.length ()), iid (_iid.c_str (), _iid.length ());
		SYNC_BEGIN (&singleton_.sync_domain_);
		return singleton_.bind_interface_sync (name, iid);
		SYNC_END ();
	}

	/// Bind ClassLibrary.
	/// 
	/// \param mod ClassLibrary object.
	/// \returns The ModuleInit interface pointer or `nil` if not present.
	static ModuleInit::_ptr_type bind (ClassLibrary& mod);

	/// Bind Singleton.
	/// 
	/// \param mod Singleton object.
	/// \returns The ModuleInit interface pointer or `nil` if not present.
	static ModuleInit::_ptr_type bind (Singleton& mod);

	/// Bind legacy process executable.
	/// 
	/// \param mod Module interface.
	/// \param metadata Module OLF metadata section.
	/// \returns The Main interface pointer.
	static Legacy::Main::_ptr_type bind_executable (::Nirvana::Module::_ptr_type mod, const Section& metadata)
	{
		SYNC_BEGIN (&singleton_.sync_domain_);
		const ModuleStartup* startup = singleton_.module_bind (mod, metadata, nullptr);
		try {
			if (!startup || !startup->startup)
				invalid_metadata ();
			return Legacy::Main::_check (startup->startup);
		} catch (...) {
			singleton_.module_unbind (mod, metadata);
			throw;
		}
		SYNC_END ();
	}

	/// Unbind module.
	/// 
	/// \param mod Module interface.
	/// \param metadata Module OLF metadata section.
	static void unbind (::Nirvana::Module::_ptr_type mod, const Section& metadata) NIRVANA_NOEXCEPT
	{
		SYNC_BEGIN (&singleton_.sync_domain_);
		singleton_.module_unbind (mod, metadata);
		SYNC_END ();
	}

private:

	/// Bind module.
	/// 
	/// \param mod The Nirvana::Module interface.
	/// \param metadata Module metadata.
	/// \param sync_context Synchronization context.
	///   If module is Legacy::Executable, sync_context must be `nullptr`.
	///   If module is ClassLibrary, sync_context is free context.
	///   If module is Singleton, sync_context is the singleton synchronization domain.
	/// 
	/// \returns Pointer to the ModuleStartup metadata structure, if found. Otherwise `nullptr`.
	const ModuleStartup* module_bind (::Nirvana::Module::_ptr_type mod, const Section& metadata, SyncContext* sync_context);
	void module_unbind (::Nirvana::Module::_ptr_type mod, const Section& metadata) NIRVANA_NOEXCEPT;

	class OLF_Iterator;

	void export_add (const char* name, InterfacePtr itf);
	void export_remove (const char* name) NIRVANA_NOEXCEPT;

	InterfacePtr bind_interface_sync (const char* name, size_t name_len, const char* iid, size_t iid_len);

	InterfacePtr bind_interface_sync (const CoreString& name, const CoreString& iid)
	{
		return bind_interface_sync (name.c_str (), name.length (), iid.c_str (), iid.length ());
	}

	InterfacePtr bind_interface_sync (const char* name, const char* iid)
	{
		return bind_interface_sync (name, strlen (name), iid, strlen (iid));
	}

	CORBA::Object::_ptr_type bind_sync (const CoreString& name)
	{
		return bind_sync (name.c_str (), name.length ());
	}

	CORBA::Object::_ptr_type bind_sync (const char* name)
	{
		return bind_sync (name, strlen (name));
	}

	CORBA::Object::_ptr_type bind_sync (const char* name, size_t name_len);

	InterfacePtr find (const char* name, size_t name_len) const;

	NIRVANA_NORETURN static void invalid_metadata ();

private:
	ImplStatic <SyncDomain> sync_domain_;
	Map map_;

	static Binder singleton_;
};

}
}

#endif
