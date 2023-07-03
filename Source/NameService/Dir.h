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

class DirBase
{
public:
	static PortableServer::POA::_ref_type _default_POA ()
	{
		return FileSystem::adapter ();
	}

	// NamingContext::destroy () does nothing
	static void _s_destroy (CORBA::Internal::Bridge <CosNaming::NamingContext>*,
		CORBA::Internal::Interface* _env)
	{}

	static CORBA::Internal::Interface* _s_new_context (CORBA::Internal::Bridge <CosNaming::NamingContext>*,
		CORBA::Internal::Interface* _env)
	{
		CORBA::Internal::set_BAD_OPERATION (_env);
		return nullptr;
	}

};

template <class Impl>
class DirImpl :
	public CORBA::servant_traits <Nirvana::Dir>::Servant <Impl>,
	public DirBase
{
public:
	using DirBase::_default_POA;
	using DirBase::_s_destroy;
	using DirBase::_s_new_context;
};

class Dir : public DirImpl <Dir>,
	protected Port::Dir
{
	typedef Port::Dir Base;

public:
	template <class ... Args>
	Dir (Args ... args) :
		Base (std::forward <Args> (args)...)
	{}

	bool _non_existent ()
	{
		return type () == Nirvana::FileType::not_found;
	}

	FileType type () noexcept
	{
		return Base::type ();
	}

	void stat (FileStat& st) const
	{
		Base::stat (st);
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
		check_name (n);
		return FileSystem::get_reference (resolve_path (n));
	}

	void unbind (CosNaming::Name& n)
	{
		check_name (n);
		Base::unlink (n);
	}

	CosNaming::NamingContext::_ref_type bind_new_context (CosNaming::Name& n)
	{
		return FileSystem::get_dir (create_dir (n));
	}

	using NamingContextRoot::list;

	Access::_ref_type open (CosNaming::Name& n, uint_fast16_t flags, uint_fast16_t mode)
	{
		check_name (n);
		return FileSystem::get_file (get_new_file_id (n))->open (flags, mode);
	}

	Access::_ref_type mkstemps (IDL::String& name, size_t suffix_len)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	DirIterator::_ref_type opendir (uint_fast16_t flags)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

};

}
}

#endif
