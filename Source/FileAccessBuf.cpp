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
#include "MemContext.h"

namespace Nirvana {
namespace Core {

FileAccessBufBase::FileAccessBufBase () noexcept :
	dirty_begin_ (std::numeric_limits <size_t>::max ()),
	dirty_end_ (0),
	event_ (true)
{}

FileAccessBufBase::LockEvent::LockEvent (FileAccessBufBase& obj) noexcept :
	event_ (obj.event_)
{
	while (!event_.signalled ())
		event_.wait ();
	event_.reset ();
}

/// When the buffer is not empty, we assume that the value owns the file read lock on this buffer.
/// When object is copied, the buffer is not. The copy is not own any lock initially.
FileAccessBuf::FileAccessBuf (const FileAccessBuf& src) :
	Servant (position (), access (), Bytes (), 0, block_size (), flags (), eol ())
{}

void FileAccessBuf::check () const
{
	if (!access ())
		throw_BAD_PARAM (make_minor_errno (EBADF));
}

void FileAccessBuf::check (const void* p) const
{
	check ();
	if (!p)
		throw_BAD_PARAM (make_minor_errno (EFAULT));
}

void FileAccessBuf::check_read () const
{
	if ((flags () & O_ACCMODE) == O_WRONLY)
		throw_NO_PERMISSION (make_minor_errno (EINVAL));
}

void FileAccessBuf::check_write () const
{
	if ((flags () & O_ACCMODE) == O_RDONLY)
		throw_NO_PERMISSION (make_minor_errno (EINVAL));
}

const void* FileAccessBuf::get_buffer_read_internal (size_t& cb)
{
	if (!cb)
		return nullptr;

	if (sizeof (size_t) > sizeof (uint32_t) && cb > std::numeric_limits <uint32_t>::max ())
		throw_IMP_LIMIT (make_minor_errno (ENOBUFS));

	FileSize read_end = position () + cb;
	FileSize buffer_end = buf_pos () + buffer ().size ();
	if (position () < buf_pos () || buffer_end < read_end) {
		// We need to read some data
		
		LockEvent lock_event (*this);
		check ();

		FileSize new_buf_pos = round_down (position (), (FileSize)block_size ());
		FileSize new_buf_end = round_up (position () + cb, (FileSize)block_size ());
		if (new_buf_end - new_buf_pos < block_size () * 2) {
			if (!buffer ().empty () && buf_pos () < new_buf_pos && buffer_end >= new_buf_pos)
				new_buf_pos -= block_size ();
			else
				new_buf_end += block_size ();
		}

		bool lock = lock_on_demand ();
		if (new_buf_pos >= buffer_end || new_buf_end <= buf_pos ()) {
			// Drop old buffer completely
			FileLock release;
			if (lock && !buffer ().empty ()) {
				release.start (buf_pos ());
				release.len (buffer ().size ());
				release.type (LockType::LOCK_READ);
			}
			uint32_t cb_read = (uint32_t)(new_buf_end - new_buf_pos);
			if (buffer ().empty ())
				access ()->read (release, new_buf_pos, cb_read, lock, buffer ());
			else {
				Bytes new_buf;
				access ()->read (release, new_buf_pos, cb_read, lock, new_buf);
				buffer ().swap (new_buf);
			}
		} else {
			// Extend buffer
			if (new_buf_pos < buf_pos ()) {
				// Extend before
				FileLock release;
				size_t drop_tail = 0;
				if (buffer_end > new_buf_end) {
					drop_tail = (size_t)(buffer_end - new_buf_end);
					if (lock) {
						release.start (new_buf_end);
						release.len (drop_tail);
						release.type (LockType::LOCK_READ);
					}
				}
				Bytes new_buf;
				access ()->read (release, new_buf_pos, (uint32_t)(buf_pos () - new_buf_pos), lock, new_buf);
				if (drop_tail)
					buffer ().resize (buffer ().size () - drop_tail);
				new_buf.insert (new_buf.end (), buffer ().data (), buffer ().data () + buffer ().size ());
				buffer ().swap (new_buf);
				buf_pos (new_buf_pos);
			}
			if (buffer_end < new_buf_end) {
				// Extend after
				FileLock release;
				size_t drop_front = 0;
				if (buf_pos () < new_buf_pos) {
					drop_front = new_buf_pos - buf_pos ();
					if (lock) {
						release.start (buf_pos ());
						release.len (new_buf_pos - buf_pos ());
						release.type (LockType::LOCK_READ);
					}
				}
				Bytes new_buf;
				access ()->read (release, buffer_end, (uint32_t)(new_buf_end - buffer_end), lock, new_buf);
				buffer ().insert (buffer ().end (), new_buf.data (), new_buf.data () + new_buf.size ());
			}
		}
	}

	size_t off = (size_t)(position () - buf_pos ());
	size_t max = buffer ().size () - off;
	if (cb > max)
		cb = max;
	return buffer ().data () + off;
}

void FileAccessBuf::flush ()
{
	check ();
	if (dirty ()) {
		if (dirty_begin_ == 0 && dirty_end_ == buffer ().size ())
			access ()->write (buf_pos (), buffer ());
		else {
			Bytes tmp (buffer ().begin () + dirty_begin_, buffer ().begin () + dirty_end_);
			access ()->write (buf_pos () + dirty_begin_, tmp);
		}
		dirty_begin_ = std::numeric_limits <size_t>::max ();
		dirty_end_ = 0;
	}
}

}
}

NIRVANA_VALUETYPE_IMPL (Nirvana::AccessBuf, Nirvana::Core::FileAccessBuf)
