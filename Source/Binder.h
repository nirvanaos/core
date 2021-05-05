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

#include "core.h"
#include <CORBA/Server.h>
#include <Binder_s.h>
#include "SyncDomain.h"
#include "Section.h"
#include "Synchronized.h"
#include "Heap.h"
#include <CORBA/RepositoryId.h>
#include <Nirvana/Main.h>

#pragma push_macro ("verify")
#undef verify
#include "parallel-hashmap/parallel_hashmap/btree.h"
#pragma pop_macro ("verify")

namespace Nirvana {
namespace Core {

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

	static InterfaceRef bind (const std::string& _name, const std::string& _iid)
	{
		CoreString name (_name.c_str (), _name.length ()), iid (_iid.c_str (), _iid.length ());
		SYNC_BEGIN (&sync_domain_);
		return bind_sync (name, iid);
		SYNC_END ();
	}

	/// Bind legacy process executable.
	/// 
	/// \param mod Module interface.
	/// \param metadata Module OLF metadata section.
	/// \returns The Main interface pointer.
	static ::Nirvana::Legacy::Main::_ptr_type bind_executable (Module::_ptr_type mod, const Section& metadata)
	{
		CORBA::Nirvana::Interface* startup = module_bind (mod, metadata, nullptr);
		try {
			if (!startup)
				invalid_metadata ();
			return ::Nirvana::Legacy::Main::_check (startup);
		} catch (...) {
			module_unbind (mod, metadata);
			throw;
		}
	}

	/// Unbind module.
	/// 
	/// \param mod Module interface.
	/// \param metadata Module OLF metadata section.
	static void module_unbind (Module::_ptr_type mod, const Section& metadata) NIRVANA_NOEXCEPT;

private:
	static CORBA::Nirvana::Interface* module_bind (Module::_ptr_type mod, const Section& metadata, SyncContext* sync_context);

	class OLF_Iterator;

	static void export_add (const char* name, InterfacePtr itf);
	static void export_remove (const char* name) NIRVANA_NOEXCEPT;

	static InterfaceRef bind_sync (const CoreString& name, const CoreString& iid)
	{
		return bind_sync (name.c_str (), name.length (), iid.c_str (), iid.length ());
	}

	static InterfaceRef bind_sync (const char* name, const char* iid)
	{
		return bind_sync (name, strlen (name), iid, strlen (iid));
	}

	static InterfaceRef bind_sync (const char* name, size_t name_len, const char* iid, size_t iid_len);

	NIRVANA_NORETURN static void invalid_metadata ();

private:
	static ImplStatic <SyncDomain> sync_domain_;
	static Map map_;
};

}
}

#endif
