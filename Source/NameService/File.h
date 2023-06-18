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
#include "../FileAccessDirect.h"
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

	File (const DirItemId& id) :
		Base (id)
	{
		assert (Base::type () != FileType::directory);
	}

	FileType type () const noexcept
	{
		return Base::type ();
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

	uint64_t size () const
	{
		if (access_)
			return access_->size ();
		else
			return Base::size ();
	}

	Nirvana::FileAccess::_ref_type open (unsigned flags)
	{
		if (!access_)
			access_ = CORBA::make_reference <Nirvana::Core::FileAccessDirect> (std::ref (port ()), flags);
		return access_->_this ();
	}

private:
	CORBA::servant_reference <Nirvana::Core::FileAccessDirect> access_;
};

}
}

#endif
