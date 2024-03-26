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
#include "pch.h"
#include "FileAccessBuf.h"
#include "MemContext.h"

namespace Nirvana {
namespace Core {

FileAccessBufData::FileAccessBufData () noexcept :
	dirty_begin_ (std::numeric_limits <size_t>::max ()),
	dirty_end_ (0),
	event_ (true)
{}

FileAccessBufData::LockEvent::LockEvent (FileAccessBufData& obj) noexcept :
	event_ (obj.event_)
{
	while (!event_.signalled ())
		event_.wait ();
	event_.reset ();
}

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
		throw_NO_PERMISSION (make_minor_errno (EBADF));
}

void FileAccessBuf::check_write () const
{
	if ((flags () & O_ACCMODE) == O_RDONLY)
		throw_NO_PERMISSION (make_minor_errno (EBADF));
}

Nirvana::AccessDirect::_ptr_type FileAccessBuf::direct ()
{
	check ();
	if (private_flags () & ACCESS_COPIED) {
		private_flags () &= ~ACCESS_COPIED;
		access (AccessDirect::_narrow (access ()->dup (0, 0)->_to_object ()));
	}
	return access ();
}

const void* FileAccessBuf::get_buffer_read_internal (size_t& cb)
{
	assert (cb);

	if (sizeof (size_t) > sizeof (uint32_t) && cb > std::numeric_limits <uint32_t>::max ())
		throw_IMP_LIMIT (make_minor_errno (ENOBUFS));

	FileSize read_end = position () + cb;
	FileSize buffer_end = buf_pos () + buffer ().size ();
	if (position () < buf_pos () || buffer_end < read_end) {
		// The read range does not fit in the current buffer
		// We need to read some data
		
		LockEvent lock_event (*this);
		check ();

		// New buffer position
		BufPos new_buf_pos;
		get_new_buf_pos (cb, new_buf_pos);

		if (new_buf_pos.begin >= buffer_end || new_buf_pos.end <= buf_pos ()) {
			// Drop old buffer completely
			if (dirty ())
				flush_internal ();
			uint32_t cb_read = (uint32_t)(new_buf_pos.end - new_buf_pos.begin);
			if (buffer ().empty ())
				direct ()->read (new_buf_pos.begin, cb_read, buffer ());
			else {
				Bytes new_buf;
				direct ()->read (new_buf_pos.begin, cb_read, new_buf);
				buffer ().swap (new_buf);
			}
			buf_pos (new_buf_pos.begin);

		} else {
			// Extend/shift buffer
			assert (!buffer ().empty ());

			// If dropped parts of the buffer are dirty, save dirty part of buffer
			flush_buf_shift (new_buf_pos);

			if (new_buf_pos.begin < buf_pos ()) {
				// Extend before
				FileLock release;
				size_t drop_tail = 0;
				if (buffer_end > new_buf_pos.end)
					drop_tail = (size_t)(buffer_end - new_buf_pos.end);
				Bytes new_buf;
				uint32_t cb_before = (uint32_t)(buf_pos () - new_buf_pos.begin);
				direct ()->read (new_buf_pos.begin, cb_before, new_buf);
				buffer ().resize (buffer ().size () - drop_tail);
				new_buf.insert (new_buf.end (), buffer ().data (), buffer ().data () + buffer ().size ());
				buffer ().swap (new_buf);
				if (dirty ()) {
					dirty_begin_ += cb_before;
					dirty_end_ += cb_before;
				}
				buf_pos (new_buf_pos.begin);
			}
			if (buffer_end < new_buf_pos.end) {
				// Extend after
				size_t drop_front = 0;
				if (buf_pos () < new_buf_pos.begin)
					drop_front = (size_t)(new_buf_pos.begin - buf_pos ());
				Bytes new_buf;
				uint32_t cb_after = (uint32_t)(new_buf_pos.end - buffer_end);
				direct ()->read (buffer_end, cb_after, new_buf);
				if (drop_front) {
					ptrdiff_t add = new_buf.size () - drop_front;
					if (add > 0)
						buffer ().reserve (buffer ().size () + add);
					buffer ().erase (buffer ().begin (), buffer ().begin () + drop_front);
				}
				buffer ().insert (buffer ().end (), new_buf.data (), new_buf.data () + new_buf.size ());
				buf_pos (new_buf_pos.begin);
			}
		}
	}

	size_t off = (size_t)(position () - buf_pos ());
	size_t max = buffer ().size () - off;
	if (cb > max)
		cb = max;
	return buffer ().data () + off;
}

void FileAccessBuf::get_new_buf_pos (size_t cb, BufPos& new_buf_pos) const
{
	new_buf_pos.begin = round_down (position (), (FileSize)block_size ());
	new_buf_pos.end = round_up (position () + cb, (FileSize)block_size ());
	if (new_buf_pos.end - new_buf_pos.begin < block_size () * 2) {
		if (!buffer ().empty () && buf_pos () < new_buf_pos.begin
			&& buf_pos () + buffer ().size () >= new_buf_pos.begin
		)
			new_buf_pos.begin -= block_size ();
		else
			new_buf_pos.end += block_size ();
	}
}

void FileAccessBuf::flush_buf_shift (const BufPos& new_buf_pos)
{
	// If dropped parts of the buffer are dirty, save them
	if (dirty ()) {
		bool save = false;
		FileSize buffer_end = buf_pos () + buffer ().size ();
		if (buffer_end > new_buf_pos.end) {
			size_t drop_tail = (size_t)(buffer_end - new_buf_pos.end);
			size_t new_buf_size = buffer ().size () - drop_tail;
			if (dirty_end_ > new_buf_size)
				save = true;
		}
		if (!save && (buf_pos () < new_buf_pos.begin)) {
			size_t drop_head = (size_t)(new_buf_pos.begin - buf_pos ());
			if (dirty_begin_ < drop_head)
				save = true;
		}

		if (save)
			flush_internal ();
	}
}

void* FileAccessBuf::get_buffer_write_internal (size_t cb)
{
	assert (cb);

	if (sizeof (size_t) > sizeof (uint32_t) && cb > std::numeric_limits <uint32_t>::max ())
		throw_IMP_LIMIT (make_minor_errno (ENOBUFS));

	if ((flags () & O_ACCMODE) != O_WRONLY) {
		size_t cbr = cb;
		get_buffer_read_internal (cbr);
	}

	FileSize write_end = position () + cb;
	FileSize buffer_end = buf_pos () + buffer ().size ();
	if (position () < buf_pos () || buffer_end < write_end) {
		// The write range does not fit in the current buffer

		LockEvent lock_event (*this);
		check ();

		// New buffer position
		BufPos new_buf_pos;
		get_new_buf_pos (cb, new_buf_pos);

		if (new_buf_pos.begin >= buffer_end || new_buf_pos.end <= buf_pos ()) {
			// Drop old buffer completely
			if (dirty ())
				flush_internal ();

			size_t new_size = (size_t)(new_buf_pos.end - new_buf_pos.begin);
			buffer ().assign (new_size, 0);

		} else {
			// Extend/shift buffer
			assert (!buffer ().empty ());

			// If dropped parts of the buffer are dirty, save dirty part of buffer
			flush_buf_shift (new_buf_pos);

			// Extend buffer
			if (new_buf_pos.begin < buf_pos ()) {
				// Extend before
				size_t drop_tail = 0;
				if (buffer_end > new_buf_pos.end)
					drop_tail = (size_t)(buffer_end - new_buf_pos.end);
				uint32_t cb_before = (uint32_t)(buf_pos () - new_buf_pos.begin);
				buffer ().resize (buffer ().size () - drop_tail);
				buffer ().insert (buffer ().begin (), cb_before, 0);
				buf_pos (new_buf_pos.begin);
			}

			if (buffer_end < new_buf_pos.end) {
				// Extend after
				uint32_t cb_after = (uint32_t)(new_buf_pos.end - buffer_end);
				buffer ().insert (buffer ().end (), cb_after, 0);
			}
		}
	}

	size_t off = (size_t)(position () - buf_pos ());
	size_t end = off + cb;
	if (dirty_begin_ > off)
		dirty_begin_ = off;
	if (dirty_end_ < end)
		dirty_end_ = end;
	return buffer ().data () + off;
}

void FileAccessBuf::flush_internal (bool sync)
{
	assert (dirty ());
	if (!sync)
		sync = flags () & O_SYNC;

	if (dirty_begin_ == 0 && dirty_end_ == buffer ().size ())
		direct ()->write (buf_pos (), buffer (), sync);
	else {
		Bytes tmp (buffer ().data () + dirty_begin_, buffer ().data () + dirty_end_);
		direct ()->write (buf_pos () + dirty_begin_, tmp, sync);
	}
	reset_dirty ();
}

uint_fast16_t FileAccessBuf::check_flags (uint_fast16_t f) const
{
	if (f & O_DIRECT)
		throw_INV_FLAG (make_minor_errno (EINVAL));

	uint_fast16_t changes = flags () ^ f;
	if (changes & ~(O_APPEND | O_SYNC | O_TEXT | O_NONBLOCK | O_ACCMODE))
		throw_INV_FLAG (make_minor_errno (EINVAL));

	return changes;
}

}
}

NIRVANA_GET_FACTORY (Nirvana::AccessBuf, Nirvana::Core::FileAccessBuf)
