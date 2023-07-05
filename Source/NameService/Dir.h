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
#include "../deactivate_servant.h"
#include "../Chrono.h"
#include <fnctl.h>

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

	Nirvana::DirItem::_ref_type resolve (CosNaming::Name& n)
	{
		check_name (n);
		return FileSystem::get_reference (Base::resolve_path (n));
	}

	void unbind (CosNaming::Name& n)
	{
		check_name (n);
		Base::unlink (n);
	}

	CosNaming::NamingContext::_ref_type bind_new_context (CosNaming::Name& n)
	{
		return FileSystem::get_dir (Base::create_dir (n));
	}

	using NamingContextRoot::list;

	Access::_ref_type open (CosNaming::Name& n, uint_fast16_t flags, uint_fast16_t mode)
	{
		check_name (n);
		if (flags & O_CREAT) {
			try {
				return FileSystem::get_file (get_new_file_id (n))->open (flags & O_EXCL, mode);
			} catch (const RuntimeError& err) {
				if ((flags & O_EXCL) || err.error_number () != EEXIST)
					throw;
			}
		}
		DirItemId id = Base::resolve_path (n);
		if (FileSystem::get_item_type (id) == FileType::directory)
			throw RuntimeError (EISDIR);
		return FileSystem::get_file (id)->open (flags, mode);
	}

	Access::_ref_type mkostemps (IDL::String& name, uint_fast16_t suffix_len, uint_fast16_t flags)
	{
		// Check name pattern
		size_t name_size = name.size ();
		if (name_size < 6 + suffix_len)
			throw RuntimeError (EINVAL);
		size_t pattern_end = name_size - suffix_len;
		size_t pattern_start = pattern_end - 6;
		char* name_p = name.data ();
		for (auto p = name_p + pattern_start, end = name_p + pattern_end; p != end; ++p) {
			if ('X' != *p)
				throw RuntimeError (EINVAL);
		}

		const int MAX_CNT = 10;

		CosNaming::Name n;
		n.emplace_back ();
		size_t ext = name.rfind ('.');
		n.front ().id (name.substr (0, ext));
		if (ext != IDL::String::npos)
			n.front ().kind (name.substr (ext + 1));

		char* p_start = n.front ().id ().data ();
		char* p_end = p_start + pattern_end;
		p_start += pattern_start;

		Access::_ref_type acc;
		for (int try_cnt = 0;;) {
			uint32_t timestamp = (uint32_t)Chrono::deadline_clock ();
			for (auto p = p_start; p != p_end; ++p) {
				uint_fast16_t d = timestamp & 0x0F;
				*p = d < 10 ? '0' + d : 'A' + d - 10;
				timestamp >>= 4;
			}
			try {
				acc = FileSystem::get_file (get_new_file_id (n))->open (O_EXCL | O_CREAT | O_RDWR
					| (flags & (O_DIRECT | O_APPEND | FILE_SHARE_DENY_WRITE | FILE_SHARE_DENY_READ)), 0600);
			} catch (const RuntimeError& ex) {
				if (ex.error_number () != EEXIST || MAX_CNT == ++try_cnt)
					throw;
			}
			if (acc) {
				real_copy (p_start, p_end, name_p + pattern_start);
				return acc;
			}
		}
	}

	DirIterator::_ref_type opendir (const IDL::String& regexp, uint_fast16_t flags)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	void remove ()
	{
		Base::remove ();
		deactivate_servant (this);
	}

};

}
}

#endif
