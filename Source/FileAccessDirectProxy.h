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
	public SimpleList <FileAccessDirectProxy>::Element
{
public:
	FileAccessDirectProxy (File& file, unsigned flags);

	Nirvana::File::_ref_type file () const;

	void close ();

	uint64_t size () const
	{
		return driver_.size ();
	}

	void size (uint64_t new_size) const
	{
		driver_.size (new_size);
	}

	void read (uint64_t pos, uint32_t size, std::vector <uint8_t>& data) const
	{
		driver_.read (pos, size, data);
	}

	void write (uint64_t pos, const std::vector <uint8_t>& data) const
	{
		driver_.write (pos, data);
	}

	void flush () const
	{
		driver_.flush ();
	}

private:
	FileAccessDirect& driver_;
	File& file_;
};

}
}

#endif
