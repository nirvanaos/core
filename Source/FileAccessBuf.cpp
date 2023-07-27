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
	buf_pos_ (0),
	dirty_begin_ (std::numeric_limits <size_t>::max ()),
	dirty_end_ (0),
	event_ (true),
	lock_mode_ (LockType::LOCK_NONE)
{}

FileAccessBufBase::LockEvent::LockEvent (FileAccessBufBase& obj) noexcept :
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
		throw_NO_PERMISSION (make_minor_errno (EINVAL));
}

void FileAccessBuf::check_write () const
{
	if ((flags () & O_ACCMODE) == O_RDONLY)
		throw_NO_PERMISSION (make_minor_errno (EINVAL));
}

const void* FileAccessBuf::get_buffer_read_internal (size_t& cb, LockType new_lock_mode)
{
	assert (cb);

	if (sizeof (size_t) > sizeof (uint32_t) && cb > std::numeric_limits <uint32_t>::max ())
		throw_IMP_LIMIT (make_minor_errno (ENOBUFS));

	FileSize read_end = position () + cb;
	FileSize buffer_end = buf_pos_ + buffer_.size ();
	if (position () < buf_pos_ || buffer_end < read_end || lock_mode_ != new_lock_mode) {
		// The read range does not fit in the current buffer
		// We need to read some data
		
		LockEvent lock_event (*this);
		check ();

		// New buffer position
		BufPos new_buf_pos;
		get_new_buf_pos (cb, new_buf_pos);

		if (new_buf_pos.begin >= buffer_end || new_buf_pos.end <= buf_pos_) {
			// Drop old buffer completely
			if (dirty ())
				flush_internal ();
			FileLock release;
			if (!buffer_.empty ()) {
				release.start (buf_pos_);
				release.len (buffer_.size ());
				release.type (lock_mode_);
			}
			uint32_t cb_read = (uint32_t)(new_buf_pos.end - new_buf_pos.begin);
			if (buffer_.empty ())
				access ()->read (release, new_buf_pos.begin, cb_read, new_lock_mode, flags () & O_NONBLOCK, buffer_);
			else {
				Bytes new_buf;
				access ()->read (release, new_buf_pos.begin, cb_read, new_lock_mode, flags () & O_NONBLOCK, new_buf);
				buffer_.swap (new_buf);
			}

			lock_mode_ = new_lock_mode;

		} else {
			// Extend/shift buffer
			assert (!buffer_.empty ());

			// If dropped parts of the buffer are dirty, save dirty part of buffer
			flush_buf_shift (new_buf_pos);

			if (new_lock_mode != lock_mode_) {
				size_t drop_tail = 0, drop_head = 0;
				if (buffer_end > new_buf_pos.end)
					drop_tail = (size_t)(buffer_end - new_buf_pos.end);
				if (buf_pos_ < new_buf_pos.begin)
					drop_head = (size_t)(new_buf_pos.begin - buf_pos_);
				FileLock rel (buf_pos_, buffer_.size (), lock_mode_);
				FileLock acq (buf_pos_ + drop_head, buffer_.size () - (drop_head + drop_tail), new_lock_mode);
				access ()->lock (rel, acq, flags () & O_NONBLOCK);
				buffer_.resize (buffer_.size () - drop_tail);
				buffer_.erase (buffer_.begin (), buffer_.begin () + drop_head);
				buf_pos_ += drop_head;
				buffer_end = buf_pos_ + buffer_.size ();
				lock_mode_ = new_lock_mode;
			}

			if (new_buf_pos.begin < buf_pos_) {
				// Extend before
				FileLock release;
				size_t drop_tail = 0;
				if (buffer_end > new_buf_pos.end) {
					drop_tail = (size_t)(buffer_end - new_buf_pos.end);
					release.start (new_buf_pos.end);
					release.len (drop_tail);
					release.type (lock_mode_);
				}
				Bytes new_buf;
				uint32_t cb_before = (uint32_t)(buf_pos_ - new_buf_pos.begin);
				access ()->read (release, new_buf_pos.begin, cb_before, lock_mode_, flags () & O_NONBLOCK, new_buf);
				buffer_.resize (buffer_.size () - drop_tail);
				try {
					new_buf.insert (new_buf.end (), buffer_.data (), buffer_.data () + buffer_.size ());
				} catch (...) {
					access ()->lock (FileLock (new_buf_pos.begin, cb_before, lock_mode_), FileLock (), true);
					throw;
				}
				buffer_.swap (new_buf);
				if (dirty ()) {
					dirty_begin_ += cb_before;
					dirty_end_ += cb_before;
				}
				buf_pos_ = new_buf_pos.begin;
			}
			if (buffer_end < new_buf_pos.end) {
				// Extend after
				FileLock release;
				size_t drop_front = 0;
				if (buf_pos_ < new_buf_pos.begin) {
					drop_front = (size_t)(new_buf_pos.begin - buf_pos_);
					release.start (buf_pos_);
					release.len (drop_front);
					release.type (lock_mode_);
				}
				Bytes new_buf;
				uint32_t cb_after = (uint32_t)(new_buf_pos.end - buffer_end);
				access ()->read (release, buffer_end, cb_after, lock_mode_, flags () & O_NONBLOCK, new_buf);
				try {
					buffer_.insert (buffer_.end (), new_buf.data (), new_buf.data () + new_buf.size ());
				} catch (...) {
					access ()->lock (FileLock (buffer_end, cb_after, lock_mode_), FileLock (), true);
					throw;
				}
			}
		}
	}

	size_t off = (size_t)(position () - buf_pos_);
	size_t max = buffer_.size () - off;
	if (cb > max)
		cb = max;
	return buffer_.data () + off;
}

void FileAccessBuf::get_new_buf_pos (size_t cb, BufPos& new_buf_pos) const
{
	new_buf_pos.begin = round_down (position (), (FileSize)block_size ());
	new_buf_pos.end = round_up (position () + cb, (FileSize)block_size ());
	if (new_buf_pos.end - new_buf_pos.begin < block_size () * 2) {
		if (!buffer_.empty () && buf_pos_ < new_buf_pos.begin
			&& buf_pos_ + buffer_.size () >= new_buf_pos.begin
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
		assert (LockType::LOCK_WRITE == lock_mode_);

		bool save = false;
		FileSize buffer_end = buf_pos_ + buffer_.size ();
		if (buffer_end > new_buf_pos.end) {
			size_t drop_tail = (size_t)(buffer_end - new_buf_pos.end);
			size_t new_buf_size = buffer_.size () - drop_tail;
			if (dirty_end_ > new_buf_size)
				save = true;
		}
		if (!save && (buf_pos_ < new_buf_pos.begin)) {
			size_t drop_head = (size_t)(new_buf_pos.begin - buf_pos_);
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
		get_buffer_read_internal (cbr, LockType::LOCK_WRITE);
	}

	FileSize write_end = position () + cb;
	FileSize buffer_end = buf_pos_ + buffer_.size ();
	if (position () < buf_pos_ || buffer_end < write_end) {
		// The write range does not fit in the current buffer

		LockEvent lock_event (*this);
		check ();

		// New buffer position
		BufPos new_buf_pos;
		get_new_buf_pos (cb, new_buf_pos);

		if (new_buf_pos.begin >= buffer_end || new_buf_pos.end <= buf_pos_) {
			// Drop old buffer completely
			if (dirty ())
				flush_internal ();

			FileLock release;
			if (!buffer_.empty ()) {
				release.start (buf_pos_);
				release.len (buffer_.size ());
				release.type (lock_mode_);
			}
			size_t new_size = (size_t)(new_buf_pos.end - new_buf_pos.begin);
			Bytes new_buf (new_size);
			access ()->lock (release, FileLock (new_buf_pos.begin, new_size, LockType::LOCK_WRITE), flags () & O_NONBLOCK);

			buffer_.swap (new_buf);
			lock_mode_ = LockType::LOCK_WRITE;

		} else {
			// Extend/shift buffer
			assert (!buffer_.empty ());

			// If dropped parts of the buffer are dirty, save dirty part of buffer
			flush_buf_shift (new_buf_pos);

			// Extend buffer
			if (new_buf_pos.begin < buf_pos_) {
				// Extend before
				FileLock release;
				size_t drop_tail = 0;
				if (buffer_end > new_buf_pos.end) {
					drop_tail = (size_t)(buffer_end - new_buf_pos.end);
					release.start (new_buf_pos.end);
					release.len (drop_tail);
					release.type (LockType::LOCK_WRITE);
				}
				uint32_t cb_before = (uint32_t)(buf_pos_ - new_buf_pos.begin);
				buffer_.resize (buffer_.size () - drop_tail);
				buffer_.insert (buffer_.begin (), cb_before, 0);
				try {
					access ()->lock (release, FileLock (new_buf_pos.begin, cb_before, LockType::LOCK_WRITE), flags () & O_NONBLOCK);
				} catch (...) {
					buffer_.erase (buffer_.begin (), buffer_.begin () + cb_before);
					throw;
				}
				buf_pos_ = new_buf_pos.begin;
			}

			if (buffer_end < new_buf_pos.end) {
				// Extend after
				FileLock release;
				if (buf_pos_ < new_buf_pos.begin) {
					release.start (buf_pos_);
					release.len (new_buf_pos.begin - buf_pos_);
					release.type (LockType::LOCK_WRITE);
				}
				uint32_t cb_after = (uint32_t)(new_buf_pos.end - buffer_end);
				buffer_.insert (buffer_.end (), cb_after, 0);
				try {
					access ()->lock (release, FileLock (buffer_end, cb_after, LockType::LOCK_WRITE), flags () & O_NONBLOCK);
				} catch (...) {
					buffer_.resize (buffer_.size () - cb_after);
					throw;
				}
			}
		}
	}

	size_t off = (size_t)(position () - buf_pos_);
	size_t end = off + cb;
	if (dirty_begin_ > off)
		dirty_begin_ = off;
	if (dirty_end_ < end)
		dirty_end_ = end;
	return buffer_.data () + off;
}

void FileAccessBuf::flush_internal (bool sync)
{
	assert (dirty ());
	if (!sync)
		sync = flags () & O_SYNC;

	FileLock fl;
	if (dirty_begin_ == 0 && dirty_end_ == buffer_.size ())
		access ()->write (buf_pos_, buffer_, fl, sync);
	else {
		Bytes tmp (buffer_.data () + dirty_begin_, buffer_.data () + dirty_end_);
		access ()->write (buf_pos_ + dirty_begin_, tmp, fl, sync);
	}
	reset_dirty ();
}

}
}

NIRVANA_VALUETYPE_IMPL (Nirvana::AccessBuf, Nirvana::Core::FileAccessBuf)
