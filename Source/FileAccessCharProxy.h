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
#ifndef NIRVANA_CORE_FILEACCESSCHARPROXY_H_
#define NIRVANA_CORE_FILEACCESSCHARPROXY_H_
#pragma once

#include <CORBA/Server.h>
#include "FileAccessChar.h"
#include <Nirvana/File_s.h>

namespace Nirvana {
namespace Core {

class FileAccessCharProxy :
	public CORBA::servant_traits <AccessChar>::Servant <FileAccessCharProxy>
{
public:
	FileAccessCharProxy (FileAccessChar& access, unsigned flags) :
		access_ (&access),
		flags_ (flags)
	{}

	Nirvana::File::_ref_type file () const
	{
		check ();
		return access_->file ();
	}

	void close ()
	{
		check ();
		access_ = nullptr;
	}

	uint_fast16_t flags () const noexcept
	{
		return flags_;
	}

	void flags (unsigned fl);

	Access::_ref_type dup () const
	{
		return CORBA::make_reference <FileAccessCharProxy> (std::ref (*this));
	}

	void read (uint32_t size, IDL::String& data)
	{
		check ();
		access_->read (size, data, !(flags_ & O_NONBLOCK));
	}
	
	void write (const IDL::String& data)
	{
		check ();
		access_->write (data);
	}

private:
	void check () const
	{
		if (!access_)
			throw_BAD_PARAM (make_minor_errno (EBADF));
	}

private:
	Ref <FileAccessChar> access_;
	unsigned flags_;
};

}
}
#endif
