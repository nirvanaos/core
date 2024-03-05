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

#include <CORBA/Server.h>
#include <Port/Dir.h>
#include "NamingContextRoot.h"
#include "FileSystem.h"
#include "../Chrono.h"
#include <Nirvana/posix.h>
#include "DirIter.h"

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

	bool _non_existent () noexcept
	{
		if (Base::type () == FileType::not_found)
			return true;

		if (Base::_non_existent ()) {
			etherealize ();
			return true;
		}

		return false;
	}

	FileType type () noexcept
	{
		return Base::type ();
	}

	void stat (FileStat& st)
	{
		check_exist ();

		try {
			Base::stat (st);
		} catch (const CORBA::OBJECT_NOT_EXIST&) {
			etherealize ();
			throw;
		}
	}

	uint_fast16_t access ()
	{
		check_exist ();
		try {
			return Base::access ();
		} catch (const CORBA::OBJECT_NOT_EXIST&) {
			etherealize ();
			throw;
		}
	}

	void remove ()
	{
		check_exist ();
		Base::remove ();
		etherealize ();
	}

	const DirItemId& id () const noexcept
	{
		return Base::id ();
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
		try {
			return resolve_const (n);
		} catch (const CORBA::OBJECT_NOT_EXIST&) {
			Base::etherealize ();
			throw;
		}
	}

	// This method must be really const to avoid race condition in iterator.
	Nirvana::DirItem::_ref_type resolve_const (CosNaming::Name& n) const
	{
		return FileSystem::get_reference (Base::resolve_path (n));
	}

	void unbind (CosNaming::Name& n)
	{
		check_name (n);
		Base::unlink (n);
	}

	CosNaming::NamingContext::_ref_type bind_new_context (CosNaming::Name& n)
	{
		check_name (n);
		try {
			return FileSystem::get_dir (Base::create_dir (n));
		} catch (const CORBA::OBJECT_NOT_EXIST&) {
			etherealize ();
			throw;
		}
	}

	void list (uint32_t how_many, CosNaming::BindingList& bl, CosNaming::BindingIterator::_ref_type& bi)
	{
		check_exist ();
		try {
			Base::list (how_many, bl, bi);
		} catch (const CORBA::OBJECT_NOT_EXIST&) {
			etherealize ();
			throw;
		}
	}

	using Base::make_iterator;

	Access::_ref_type open (CosNaming::Name& n, uint_fast16_t flags, uint_fast16_t mode)
	{
		check_name (n);
		for (;;) {
			if (flags & O_CREAT) {
				try {
					return FileSystem::get_file (get_new_file_id (n))->open (flags, mode);
				} catch (const CORBA::SystemException& err) {
					if ((flags & O_EXCL) || get_minor_errno (err.minor ()) != EEXIST)
						throw;
				}
			}

			Nirvana::File::_ref_type file = Nirvana::File::_narrow (resolve (n));
			if (!file)
				throw_BAD_OPERATION (make_minor_errno (EISDIR));

			try {
				return file->open (flags, mode);
			} catch (const CORBA::SystemException& err) {
				if (!(flags & O_CREAT) || get_minor_errno (err.minor ()) != ENOENT)
					throw;
			}
		}
	}

	Access::_ref_type mkostemps (IDL::String& name, uint_fast16_t suffix_len, uint_fast16_t flags, mode_t mode)
	{
		check_exist ();

		if (!mode)
			mode = S_IRUSR | S_IWUSR;

		// Check name pattern
		size_t name_size = name.size ();
		if (name_size < (size_t)6 + (size_t)suffix_len)
			throw_BAD_PARAM (make_minor_errno (EINVAL));
		size_t pattern_end = name_size - suffix_len;
		size_t pattern_start = pattern_end - 6;
		auto name_p = name.begin ();
		for (auto p = name_p + pattern_start, end = name_p + pattern_end; p != end; ++p) {
			if ('X' != *p)
				throw_BAD_PARAM (make_minor_errno (EINVAL));
		}

		const int MAX_CNT = 10;

		CosNaming::Name n;
		n.emplace_back ();
		size_t ext = name.rfind ('.');
		n.front ().id (name.substr (0, ext));
		if (ext != IDL::String::npos)
			n.front ().kind (name.substr (ext + 1));

		auto p_start = n.front ().id ().begin ();
		auto p_end = p_start + pattern_end;
		p_start += pattern_start;

		Access::_ref_type acc;
		for (int try_cnt = 0;;) {
			uint32_t timestamp = (uint32_t)Chrono::deadline_clock ();
			for (auto p = p_start; p != p_end; ++p) {
				unsigned d = timestamp % 36;
				timestamp /= 36;
				*p = d < 10 ? '0' + d : 'a' + d - 10;
			}
			try {
				acc = FileSystem::get_file (get_new_file_id (n))->open (O_EXCL | O_CREAT | O_RDWR
					| (flags & (O_DIRECT | O_APPEND)), mode);
			} catch (const CORBA::OBJECT_NOT_EXIST&) {
				etherealize ();
				throw;
			} catch (const CORBA::SystemException& ex) {
				if (get_minor_errno (ex.minor ()) != EEXIST || MAX_CNT == ++try_cnt)
					throw;
			}
			if (acc) {
				std::copy (p_start, p_end, name_p + pattern_start);
				return acc;
			}
		}
	}

	void opendir (const IDL::String& regexp, unsigned flags,
		uint32_t how_many, DirEntryList& l, DirIterator::_ref_type& di);

private:
	void check_name (const CosNaming::Name& n);
	void check_exist ();
	void bind_file (CosNaming::Name& n, CORBA::Object::_ptr_type obj, bool rebind);
	void bind_dir (CosNaming::Name& n, CORBA::Object::_ptr_type obj, bool rebind);
	void etherealize ();
};

inline
DirIter::DirIter (Dir& dir, const std::string& regexp, unsigned flags) :
	dir_ (&dir),
	iterator_ (dir.make_iterator ()),
	flags_ (flags & ~USE_REGEX)
{
	if (!regexp.empty () && regexp != "*" && regexp != "*.*") {
		flags_ |= USE_REGEX;
		throw_NO_IMPLEMENT (make_minor_errno (ENOSYS)); // TODO: Implement.
	}
}

inline
void Dir::opendir (const IDL::String& regexp, unsigned flags,
	uint32_t how_many, DirEntryList& l, DirIterator::_ref_type& di)
{
	check_exist ();
	std::unique_ptr <DirIter> iter;
	try {
		iter = std::make_unique <DirIter> (std::ref (*this), std::ref (regexp), flags);
	} catch (const CORBA::OBJECT_NOT_EXIST&) {
		etherealize ();
		throw;
	}
	iter->next_n (how_many, l);
	if (!iter->end ())
		di = DirIter::create_iterator (std::move (iter));
}

}
}

#endif
