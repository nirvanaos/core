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
#ifndef NIRVANA_CORE_FILEACCESSDIRECTPROXY_H_
#define NIRVANA_CORE_FILEACCESSDIRECTPROXY_H_
#pragma once

#include "FileAccessDirect.h"
#include <Nirvana/File_s.h>
#include <Nirvana/SimpleList.h>

namespace Nirvana {
namespace Core {

class File;

class FileAccessDirectProxy :
	public CORBA::servant_traits <AccessDirect>::Servant <FileAccessDirectProxy>,
	public FileAccessBase,
	public SimpleList <FileAccessDirectProxy>::Element
{
public:
	FileAccessDirectProxy (File& file, unsigned access_mask);
	~FileAccessDirectProxy ();

	bool _non_existent () const noexcept
	{
		return !file_;
	}

	Nirvana::File::_ref_type file () const;

	void close ();

	FileSize size () const
	{
		if (!file_)
			throw CORBA::OBJECT_NOT_EXIST ();

		return driver_.size ();
	}

	void size (FileSize new_size) const
	{
		if (!file_)
			throw CORBA::OBJECT_NOT_EXIST ();

		if (access_mask () & AccessMask::WRITE)
			driver_.size (new_size);
		else
			throw RuntimeError (EACCES);
	}

	void read (FileSize pos, uint32_t size, std::vector <uint8_t>& data) const
	{
		if (!file_)
			throw CORBA::OBJECT_NOT_EXIST ();

		if (access_mask () & AccessMask::READ)
			driver_.read (pos, size, data);
		else
			throw RuntimeError (EACCES);
	}

	void write (FileSize pos, const std::vector <uint8_t>& data)
	{
		if (!file_)
			throw CORBA::OBJECT_NOT_EXIST ();

		if (access_mask () & AccessMask::WRITE) {
			dirty_ = true;
			driver_.write (pos, data);
		} else
			throw RuntimeError (EACCES);
	}

	void flush ()
	{
		if (!file_)
			throw CORBA::OBJECT_NOT_EXIST ();

		if (dirty_) {
			dirty_ = false;
			driver_.flush ();
		}
	}

	bool lock (FileSize start, FileSize size, short op)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

private:
	FileAccessDirect& driver_;
	Ref <File> file_;
	bool dirty_ = false;
};

}
}

#endif
