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
#ifndef NIRVANA_CORE_DIR_H_
#define NIRVANA_CORE_DIR_H_
#pragma once

#include <Port/Dir.h>
#include <Nirvana/File_s.h>
#include "NamingContextRoot.h"
#include "FileSystem.h"

namespace Nirvana {
namespace Core {

class Dir :
	public CORBA::servant_traits <Nirvana::Dir>::Servant <Dir>,
	protected Port::Dir
{
	typedef Port::Dir Base;

public:
	static PortableServer::POA::_ref_type _default_POA ()
	{
		return FileSystem::adapter ();
	}

	// NamingContext::destroy () does nothing
	static void _s_destroy (CORBA::Internal::Bridge <CosNaming::NamingContext>*,
		CORBA::Internal::Interface* _env)
	{}

	Dir (const DirItemId& id) :
		Base (id)
	{
		assert (Base::type () == FileType::directory);
	}

	static FileType type () noexcept
	{
		return FileType::directory;
	}

	uint16_t permissions () const
	{
		return Base::permissions ();
	}

	void permissions (uint16_t perms)
	{
		Base::permissions (perms);
	}

	void get_file_times (FileTimes& times) const
	{
		return Base::get_file_times (times);
	}

	void bind (CosNaming::Name& n, CORBA::Object::_ptr_type obj, bool rebind = false);

	void rebind (CosNaming::Name& n, CORBA::Object::_ptr_type obj)
	{
		bind (n, obj, true);
	}

	void bind_context (CosNaming::Name& n, CosNaming::NamingContext::_ptr_type nc, bool rebind = false);

	void rebind_context (CosNaming::Name& n, CosNaming::NamingContext::_ptr_type nc)
	{
		bind_context (n, nc, true);
	}

	CORBA::Object::_ref_type resolve (CosNaming::Name& n)
	{
		return FileSystem::get_reference (resolve_path (n));
	}

	void unbind (CosNaming::Name& n)
	{
		check_name (n);
		Base::unbind (n);
	}

	static CORBA::Internal::Interface* _s_new_context (CORBA::Internal::Bridge <CosNaming::NamingContext>*,
		CORBA::Internal::Interface* _env)
	{
		CORBA::Internal::set_BAD_OPERATION (_env);
		return nullptr;
	}

	CosNaming::NamingContext::_ref_type bind_new_context (CosNaming::Name& n)
	{
		return FileSystem::get_dir (create_dir (n));
	}

	Nirvana::FileAccess::_ref_type open (CosNaming::Name& n, unsigned short flags)
	{
		return FileSystem::open (get_new_file_id (n), flags);
	}

	using NamingContextRoot::list;
};

}
}

#endif