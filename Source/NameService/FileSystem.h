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
#include "IteratorStack.h"

namespace Nirvana {
namespace Core {

class FileSystem : private Port::FileSystem
{
public:
	FileSystem ();
	~FileSystem ();

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

	Dir::_ref_type resolve_root (const IDL::String& name)
	{
		Dir::_ref_type dir;
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

	size_t root_cnt () const noexcept
	{
		return roots_.size ();
	}

	void get_roots (CosNaming::Core::IteratorStack& iter) const
	{
		for (const auto& r : roots_) {
			iter.push (r.first, CosNaming::BindingType::ncontext);
		}
	}

	static PortableServer::POA::_ref_type adapter () noexcept
	{
		assert ((PortableServer::POA::_ref_type)adapter_);
		return adapter_;
	}

	static void set_error_number (int err);

	static CORBA::Object::_ref_type get_reference (const DirItemId& id);
	static Nirvana::Dir::_ref_type get_dir (const DirItemId& id);
	static Nirvana::File::_ref_type get_file (const DirItemId& id);

	static Nirvana::FileAccess::_ref_type open (const DirItemId& id, unsigned short flags)
	{
		return get_file (id)->open (flags);
	}

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

	static Nirvana::Core::StaticallyAllocated <PortableServer::POA::_ref_type> adapter_;
};

}
}

#endif
