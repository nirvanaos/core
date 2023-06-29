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
#ifndef NIRVANA_CORE_FILEACCESSBUF_H_
#define NIRVANA_CORE_FILEACCESSBUF_H_
#pragma once

#include <CORBA/Server.h>
#include <Nirvana/File_s.h>

namespace Nirvana {
namespace Core {

class FileAccessBuf : public CORBA::servant_traits <AccessBuf>::Servant <FileAccessBuf>
{
	typedef CORBA::servant_traits <AccessBuf>::Servant <FileAccessBuf> Base;

public:
	FileAccessBuf (Bytes&& data, AccessDirect::_ref_type&& access, FilePos pos, uint32_t block_size, unsigned flags) :
		Base (std::move (data), std::move (access), pos, block_size, flags),
		dirty_begin_ (0),
		dirty_end_ (0)
	{}

	FileAccessBuf () :
		dirty_begin_ (0),
		dirty_end_ (0)
	{}

	~FileAccessBuf ()
	{}

	Nirvana::File::_ref_type file () const
	{
		return access ()->file ();
	}

	void close ()
	{
		flush ();
		buffer ().clear ();
		access ()->close ();
		access () = nullptr;
	}

	void read (void* p, uint32_t cb)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	void write (const void* p, uint32_t cb)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	const void* get_buffer_read (uint32_t cb)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	void* get_buffer_write (uint32_t cb)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	void release_buffer (uint32_t cb)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	FilePos pos () const noexcept
	{
		return Base::position ();
	}

	FilePos seek (SeekMethod m, FileOff off)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	void flush ()
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	bool lock (FileLock& fl, short op)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	AccessDirect::_ref_type direct () const noexcept
	{
		return access ();
	}

private:
	size_t dirty_begin_, dirty_end_;
};

}
}

#endif
