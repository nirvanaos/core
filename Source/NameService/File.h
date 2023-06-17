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
#ifndef NIRVANA_FS_CORE_FILE_H_
#define NIRVANA_FS_CORE_FILE_H_
#pragma once

#include <CORBA/Server.h>
#include <Nirvana/FS_s.h>
#include <Port/File.h>
#include "../FileAccessDirect.h"
#include "FileSystem.h"

namespace Nirvana {
namespace FS {
namespace Core {

class File : public CORBA::servant_traits <Nirvana::FS::File>::Servant <File>,
	private Port::File
{
public:
	File (const PortableServer::ObjectId& id) :
		Port::File (id)
	{}

	File (const PortableServer::ObjectId& id, Nirvana::Core::FileAccessDirect& access) :
		Port::File (id),
		access_ (&access)
	{}

	static FileType type () noexcept
	{
		return FileType::regular;
	}

	uint16_t permissions () const
	{
		return Port::File::permissions ();
	}

	void permissions (uint16_t perms)
	{
		Port::File::permissions (perms);
	}

	void get_file_times (FileTimes& times) const
	{
		return Port::File::get_file_times (times);
	}

	uint64_t size () const
	{
		if (access_)
			return access_->size ();
		else
			return Port::File::size ();
	}

	CORBA::Object::_ref_type open (unsigned flags)
	{
		if (!access_)
			access_ = FileSystem::create_file_access (Port::File::id (), flags);
		return access_->_this ();
	}

private:
	CORBA::servant_reference <Nirvana::Core::FileAccessDirect> access_;
};

}
}
}
#endif
