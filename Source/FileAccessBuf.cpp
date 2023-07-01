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
#include "FileAccessBuf.h"

namespace Nirvana {
namespace Core {

void FileAccessBuf::check () const
{
	if (!access ())
		throw RuntimeError (EBADF);
}

void FileAccessBuf::check (const void* p) const
{
	check ();
	if (!p)
		throw CORBA::BAD_PARAM ();
}

void FileAccessBuf::check_read () const
{
	if ((flags () & O_ACCMODE) == O_WRONLY)
		throw RuntimeError (EINVAL);
}

void FileAccessBuf::check_write () const
{
	if ((flags () & O_ACCMODE) == O_RDONLY)
		throw RuntimeError (EINVAL);
}

void FileAccessBuf::read_next_buffer ()
{
	flush ();
	FileSize next = round_down (position (), (FileSize)block_size ()) + buffer ().size ();
	Bytes new_buf;
	access ()->read (next, block_size (), new_buf);
	buffer ().swap (new_buf);
	position (next);
}

void FileAccessBuf::set_position (FileSize pos)
{
	FileSize buf_begin = round_down (position (), (FileSize)block_size ());
	FileSize buf_end = buf_begin + buffer ().size ();
	if (pos < buf_begin || pos >= buf_end) {
		flush ();
		buffer ().clear ();
		if ((flags () & O_ACCMODE) != O_WRONLY)
			buffer ().shrink_to_fit ();
	}

	position (pos);
}

void FileAccessBuf::flush ()
{
	check ();
	if (dirty_begin_ < dirty_end_) {
		if (dirty_begin_ == 0 && dirty_end_ == buffer ().size ())
			access ()->write (buffer_position (), buffer ());
		else {
			Bytes tmp (buffer ().begin () + dirty_begin_, buffer ().begin () + dirty_end_);
			access ()->write (buffer_position () + dirty_begin_, tmp);
		}
		dirty_begin_ = std::numeric_limits <size_t>::max ();
		dirty_end_ = 0;
	}
}

}
}

NIRVANA_VALUETYPE_IMPL (Nirvana::AccessBuf, Nirvana::Core::FileAccessBuf)
