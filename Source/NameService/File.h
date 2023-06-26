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
#include "FileSystem.h"

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

	FileType type ()
	{
		return Base::type ();
	}

	uint16_t permissions ()
	{
		return Base::permissions ();
	}

	void permissions (uint16_t perms)
	{
		Base::permissions (perms);
	}

	void get_file_times (FileTimes& times)
	{
		return Base::get_file_times (times);
	}

	uint64_t size ()
	{
		if (access_)
			return access_->size ();
		else
			return Base::size ();
	}

	Access::_ref_type open (unsigned flags)
	{
		if (!access_)
			access_ = std::make_unique <Nirvana::Core::FileAccessDirect> (std::ref (port ()), flags);

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
			return CORBA::make_reference <FileAccessDirectProxy> (std::ref (*this), 
				access_mask | (deny_mask << FileAccessBase::MASK_SHIFT))->_this ();
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

inline Nirvana::File::_ref_type FileAccessDirectProxy::file () const
{
	return file_->_this ();
}

void FileAccessDirectProxy::close ()
{
	if (access_mask () & AccessMask::WRITE)
		flush ();
	remove ();
	file_->on_close_proxy ();
}

FileAccessDirectProxy::~FileAccessDirectProxy ()
{
	remove ();
	file_->on_close_proxy ();
}

}
}

#endif
