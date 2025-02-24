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
#include <CORBA/Server.h>
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
	File (Args&& ... args) :
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
			Base::stat (st);
		} catch (const CORBA::OBJECT_NOT_EXIST&) {
			etherealize ();
			throw;
		}

		if (access_)
			st.size (access_->size ());
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
		if (!(flags & (O_CREAT | O_TRUNC)))
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
		} else if ((flags & (O_EXCL | O_CREAT)) == (O_EXCL | O_CREAT))
			throw_BAD_PARAM (make_minor_errno (EEXIST));

		check_flags (flags);

		uint_fast16_t direct_flags = (flags & O_DIRECT) ? flags : (flags & ~(O_APPEND | O_TEXT | O_SYNC | O_NONBLOCK)) | O_DIRECT;

		AccessDirect::_ref_type direct;
		try {
			direct = CORBA::make_reference <FileAccessDirectProxy> (std::ref (*this), direct_flags)->_this ();
		} catch (...) {
			if (!proxy_cnt_)
				access_ = nullptr;
			throw;
		}

		Access::_ref_type ret;

		if (flags & O_DIRECT)
			ret = direct;
		else {
			Bytes buf;
			uint32_t block_size = access_->block_size ();
			if ((flags & O_ACCMODE) != O_WRONLY && access_->size ())
				direct->read (0, block_size, buf);
			ret = CORBA::make_reference <FileAccessBuf> (0, 0,
				std::move (direct), std::move (buf), block_size, flags, Port::FileSystem::eol ());
		}

		return ret;
	}

	const DirItemId& id () const noexcept
	{
		return Port::File::id ();
	}

	void on_delete_proxy (FileAccessDirectProxy* proxy) noexcept
	{
		access_->on_delete_proxy (proxy);
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
{
	assert (file.access_);
	++file.proxy_cnt_;
}

inline
FileAccessDirectProxy::~FileAccessDirectProxy ()
{
	if (file_)
		file_->on_delete_proxy (this);
}

inline
Nirvana::File::_ref_type FileAccessDirectProxy::file () const
{
	return file_->_this ();
}

inline
void FileAccessDirectProxy::set_flags (uint_fast16_t mask, uint_fast16_t f)
{
	check_exist ();
	f = (f & mask) | (flags () & ~mask);
	check_flags (f);
	flags_ = f;
}

inline
void FileAccessDirectProxy::close ()
{
	check_exist ();

	if (flags_ & O_ACCMODE)
		flush ();
	file_->on_delete_proxy (this);
	file_ = nullptr;
	deactivate_servant (this);
}

inline
Access::_ref_type FileAccessDirectProxy::dup (uint_fast16_t mask, uint_fast16_t flags) const
{
	check_exist ();
	flags &= mask;
	flags |= (flags_ & ~mask);
	check_flags (flags);
	return CORBA::make_reference <FileAccessDirectProxy> (std::ref (*file_), flags)->_this ();
}

}
}

#endif
