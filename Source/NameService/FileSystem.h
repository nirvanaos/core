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
#ifndef NIRVANA_FS_CORE_FILESYSTEM_H_
#define NIRVANA_FS_CORE_FILESYSTEM_H_
#pragma once

#include <Port/FileSystem.h>
#include "../MapUnorderedUnstable.h"
#include "../ORB/Services.h"
#include "NamingContextBase.h"
#include <Nirvana/File_s.h>

namespace PortableServer {
namespace Core {
class FileActivator;
}
}

namespace Nirvana {
namespace Core {

class FileSystem : 
	public CORBA::servant_traits <Nirvana::Dir>::Servant <FileSystem>,
	public CosNaming::Core::NamingContextBase,
	private Port::FileSystem
{
public:
	static const char adapter_name_ [];

	FileSystem ()
	{
		{ // Create adapter
			PortableServer::POA::_ref_type parent = PortableServer::POA::_narrow (
				CORBA::Core::Services::bind (CORBA::Core::Services::RootPOA));
			parent->find_POA (adapter_name_, true);
			assert (adapter ());
		}

		{ // Build root map
			Roots roots = Port::FileSystem::get_roots ();
			for (auto& r : roots) {
				roots_.emplace (std::move (r.dir), r.factory);
			}
		}
	}

	~FileSystem ()
	{}

	bool find (const IDL::String& name) const noexcept
	{
		return roots_.find (name) != roots_.end ();
	}

	static Nirvana::DirItem::FileType get_item_type (const DirItemId& id)
	{
		return Port::FileSystem::get_item_type (id);
	}

	static PortableServer::ServantBase::_ref_type incarnate (const DirItemId& id)
	{
		return Port::FileSystem::incarnate (id);
	}

	static void etherealize (const DirItemId& id, CORBA::Object::_ptr_type servant)
	{
		Port::FileSystem::etherealize (id, servant);
	}

	Nirvana::Dir::_ref_type resolve_root (const IDL::String& name)
	{
		Nirvana::Dir::_ref_type dir;
		auto it = roots_.find (name);
		if (it != roots_.end ()) {
			if (it->second.cached)
				dir = it->second.cached;
			else {
				bool cache;
				dir = get_dir ((it->second.factory) (name, cache));
				if (cache)
					it->second.cached = dir;
			}
		}
		return dir;
	}

	static PortableServer::POA::_ref_type& adapter () noexcept
	{
		assert ((PortableServer::POA::_ref_type&)adapter_);
		return adapter_;
	}

	static void set_error_number (int err);

	static CORBA::Object::_ref_type get_reference (const DirItemId& id);
	static Nirvana::Dir::_ref_type get_dir (const DirItemId& id);

	static Nirvana::File::_ref_type get_file (const DirItemId& id)
	{
		assert (get_item_type (id) != Nirvana::DirItem::FileType::directory);
		Nirvana::File::_ref_type file = Nirvana::File::_narrow (get_reference (id,
			CORBA::Internal::RepIdOf <Nirvana::File>::id));
		assert (file);
		return file;
	}

	static Access::_ref_type open (const DirItemId& id, unsigned short flags)
	{
		return get_file (id)->open (flags);
	}

	Access::_ref_type open (CosNaming::Name& n, unsigned flags)
	{
		check_name (n);
		Nirvana::Dir::_ref_type dir = resolve_root (to_string (n.front ()));
		if (dir) {
			n.erase (n.begin ());
			return dir->open (n, (uint16_t)flags);
		} else
			throw RuntimeError (ENOENT);
	}

	void destroy ()
	{
		throw NotEmpty ();
	}

	// DirItem

	static FileType type () noexcept
	{
		return FileType::directory;
	}

	uint16_t permissions () const
	{
		return 0; // TODO: Implement
	}

	void permissions (uint16_t perms)
	{
		throw CORBA::NO_IMPLEMENT (); // TODO: Implement
	}

	void get_file_times (FileTimes& times) const
	{
		zero (times);
	}

	// NamingContextBase
	virtual void bind1 (CosNaming::Istring&& name, CORBA::Object::_ptr_type obj, CosNaming::Name& n) override;
	virtual void rebind1 (CosNaming::Istring&& name, CORBA::Object::_ptr_type obj, CosNaming::Name& n) override;
	virtual void bind_context1 (CosNaming::Istring&& name, CosNaming::NamingContext::_ptr_type nc, CosNaming::Name& n) override;
	virtual void rebind_context1 (CosNaming::Istring&& name, CosNaming::NamingContext::_ptr_type nc, CosNaming::Name& n) override;
	virtual CORBA::Object::_ref_type resolve1 (const CosNaming::Istring& name, CosNaming::BindingType& type, CosNaming::Name& n) override;
	virtual void unbind1 (const CosNaming::Istring& name, CosNaming::Name& n) override;
	virtual CosNaming::NamingContext::_ref_type create_context1 (CosNaming::Istring&& name, CosNaming::Name& n, bool& created) override;
	virtual CosNaming::NamingContext::_ref_type bind_new_context1 (CosNaming::Istring&& name, CosNaming::Name& n) override;

	CosNaming::NamingContext::_ref_type new_context ()
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	// NamingContextRoot
	virtual std::unique_ptr <CosNaming::Core::Iterator> make_iterator () const override;

	// Get naming service name from file system path
	static CosNaming::Name get_name_from_path (const IDL::String& path);

private:
	static CORBA::Object::_ref_type get_reference (const DirItemId& id, CORBA::Internal::String_in iid);

private:
	struct Root {
		GetRootId factory;
		Dir::_ref_type cached;

		Root (GetRootId f) :
			factory (f)
		{}
	};

	typedef Nirvana::Core::MapUnorderedUnstable <std::string, Root> RootMap;

	RootMap roots_;

	friend class PortableServer::Core::FileActivator;
	static Nirvana::Core::StaticallyAllocated <PortableServer::POA::_ref_type> adapter_;
};

}
}

#endif
