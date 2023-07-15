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
#ifndef NIRVANA_CORE_FILE_H_
#define NIRVANA_CORE_FILE_H_
#pragma once

#include <Port/File.h>
#include <Nirvana/File_s.h>
#include "../FileAccessDirectProxy.h"
#include "../FileAccessBuf.h"
#include "FileSystem.h"
#include "../deactivate_servant.h"

namespace Nirvana {
namespace Core {

class File :
	public CORBA::servant_traits <Nirvana::File>::Servant <File>,
	protected Port::File
{
	typedef Port::File Base;

public:
	// Implementation - specific methods can be called explicitly.
	Base& port () noexcept
	{
		return *this;
	}

	static PortableServer::POA::_ref_type _default_POA ()
	{
		return FileSystem::adapter ();
	}

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

	template <class ... Args>
	File (Args ... args) :
		Base (std::forward <Args> (args)...),
		proxy_cnt_ (0)
	{}

	FileType type () noexcept
	{
		return Base::type ();
	}

	void stat (FileStat& st)
	{
		check_exist ();

		try {
			return Base::stat (st);
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

	uint64_t size ()
	{
		check_exist ();

		if (access_)
			return access_->size ();
		else {
			try {
				return Base::size ();
			} catch (const CORBA::OBJECT_NOT_EXIST&) {
				etherealize ();
				throw;
			}
		}
	}

	Access::_ref_type open (uint_fast16_t flags, uint_fast16_t mode)
	{
		bool create = (flags & (O_EXCL | O_CREAT | O_TRUNC)) == (O_EXCL | O_CREAT);
		if (!create)
			check_exist ();

		if (
			((flags & O_DIRECT) && (flags & O_TEXT))
			||
			(((flags & O_TRUNC) || (flags & O_APPEND)) && (flags & O_ACCMODE) == O_RDONLY)
			)
			throw_INV_FLAG (make_minor_errno (EINVAL));

		if (!access_) {
			try {
				access_ = std::make_unique <Nirvana::Core::FileAccessDirect> (std::ref (port ()), flags, mode);
			} catch (const CORBA::OBJECT_NOT_EXIST&) {
				etherealize ();
				throw;
			}
		} else if (create)
			throw_BAD_PARAM (make_minor_errno (EEXIST));

		check_flags (flags);

		AccessDirect::_ref_type access = CORBA::make_reference <FileAccessDirectProxy> (
			std::ref (*this), flags)->_this ();
		++proxy_cnt_;

		if (flags & O_DIRECT)
			return access;

		Bytes data;
		uint32_t block_size = access_->block_size ();
		if (!(flags & (O_APPEND | O_TRUNC | O_ATE)) && (flags & O_ACCMODE) != O_WRONLY && access_->size ())
			access_->read (0, block_size, data);
		FileSize pos = (flags & (O_APPEND | O_ATE)) ? access_->size () : 0;

		return CORBA::make_reference <FileAccessBuf> (std::move (data), access, pos, block_size, flags,
			Port::FileSystem::eol ());
	}

	void on_delete_proxy () noexcept
	{
		if (!--proxy_cnt_)
			access_ = nullptr;
	}

	void check_flags (unsigned flags) const;

private:
	void check_exist ();
	void etherealize ();

private:
	friend class FileAccessDirectProxy;

	std::unique_ptr <Nirvana::Core::FileAccessDirect> access_;
	unsigned proxy_cnt_;
};

inline
FileAccessDirectProxy::FileAccessDirectProxy (File& file, uint_fast16_t flags) :
	driver_ (*file.access_),
	file_ (&file),
	flags_ (flags),
	dirty_ (false)
{}

inline
Nirvana::File::_ref_type FileAccessDirectProxy::file () const
{
	return file_->_this ();
}

inline
void FileAccessDirectProxy::flags (uint_fast16_t f)
{
	check_exist ();

	file_->check_flags (f);
	flags_ = f;
}

inline
void FileAccessDirectProxy::close ()
{
	check_exist ();

	if (flags_ & O_ACCMODE)
		flush ();
	file_->on_delete_proxy ();
	file_ = nullptr;
	deactivate_servant (this);
}

inline
FileAccessDirectProxy::~FileAccessDirectProxy ()
{
	if (file_)
		file_->on_delete_proxy ();
}

}
}

#endif
