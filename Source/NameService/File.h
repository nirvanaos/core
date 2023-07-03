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

	template <class ... Args>
	File (Args ... args) :
		Base (std::forward <Args> (args)...)
	{}

	bool _non_existent ()
	{
		return type () == Nirvana::FileType::not_found;
	}

	Nirvana::FileType type () noexcept
	{
		return Base::type ();
	}

	void stat (FileStat& st)
	{
		return Base::stat (st);
	}

	uint64_t size ()
	{
		if (access_)
			return access_->size ();
		else
			return Base::size ();
	}

	Access::_ref_type open (uint_fast16_t flags, uint_fast16_t mode)
	{
		if (
			((flags & O_DIRECT) && (flags & O_TEXT))
			||
			(((flags & O_TRUNC) || (flags & O_APPEND)) && (flags & O_ACCMODE) == O_RDONLY)
			)
			throw CORBA::INV_FLAG ();

		if (!access_)
			access_ = std::make_unique <Nirvana::Core::FileAccessDirect> (std::ref (port ()), flags, mode);
		else if ((flags & (O_EXCL | O_CREAT | O_TRUNC)) == (O_EXCL | O_CREAT))
			throw RuntimeError (EEXIST);

		unsigned access_mask = FileAccessBase::get_access_mask (flags);
		if ((access_mask & access_->access_mask ()) != access_mask)
			throw RuntimeError (EACCES); // File is unaccessible for this mode

		unsigned deny_mask = FileAccessBase::get_deny_mask (flags);
		unsigned mask = deny_mask | (access_mask << FileAccessBase::MASK_SHIFT);
		for (auto it = proxies_.cbegin (); it != proxies_.cend (); ++it) {
			if (it->access_mask () & mask)
				throw RuntimeError (EACCES);
		}

		try {
			AccessDirect::_ref_type access = CORBA::make_reference <FileAccessDirectProxy> (std::ref (*this), 
				access_mask | (deny_mask << FileAccessBase::MASK_SHIFT))->_this ();
			if (flags & O_DIRECT)
				return access;

			Bytes data;
			uint32_t block_size = access_->block_size ();
			if (!(flags & (O_APPEND | O_TRUNC | O_ATE)) && (flags & O_ACCMODE) != O_WRONLY && access_->size ())
				access_->read (0, block_size, data);
			FileSize pos = (flags & (O_APPEND | O_ATE)) ? access_->size () : 0;

			return CORBA::make_reference <FileAccessBuf> (std::move (data), access, pos, block_size, flags);

		} catch (...) {
			if (proxies_.empty ())
				access_ = nullptr;
			throw;
		}
	}

	void on_close_proxy () noexcept
	{
		if (proxies_.empty ())
			access_ = nullptr;
	}

private:
	friend class FileAccessDirectProxy;

	std::unique_ptr <Nirvana::Core::FileAccessDirect> access_;
	SimpleList <FileAccessDirectProxy> proxies_;
};

inline
FileAccessDirectProxy::FileAccessDirectProxy (File& file, unsigned access_mask) :
	FileAccessBase (access_mask),
	driver_ (*file.access_),
	file_ (&file)
{
	file_->proxies_.push_back (*this);
}

inline
Nirvana::File::_ref_type FileAccessDirectProxy::file () const
{
	return file_->_this ();
}

inline
void FileAccessDirectProxy::close ()
{
	if (!file_)
		throw CORBA::OBJECT_NOT_EXIST ();

	if (access_mask () & AccessMask::WRITE)
		flush ();
	remove ();
	close ();
	file_->on_close_proxy ();
	file_ = nullptr;
	deactivate_servant (this);
}

inline
FileAccessDirectProxy::~FileAccessDirectProxy ()
{
	if (file_) {
		remove ();
		file_->on_close_proxy ();
	}
}

}
}

#endif
