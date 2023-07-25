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
#include <Nirvana/bitutils.h>
#include <fnctl.h>
#include "virtual_copy.h"
#include "Event.h"

namespace Nirvana {
namespace Core {

class FileAccessBufBase
{
protected:
	FileAccessBufBase () noexcept;

	FileAccessBufBase (const FileAccessBufBase&) noexcept :
		FileAccessBufBase ()
	{}

	FileAccessBufBase (FileAccessBufBase&&) noexcept :
		FileAccessBufBase ()
	{}

	class LockEvent
	{
	public:
		LockEvent (FileAccessBufBase& obj) noexcept;

		~LockEvent ()
		{
			event_.signal ();
		}

	private:
		Event& event_;
	};

	bool dirty () const noexcept
	{
		return dirty_begin_ < dirty_end_;
	}

	void reset_dirty () noexcept
	{
		dirty_begin_ = std::numeric_limits <size_t>::max ();
		dirty_end_ = 0;
	}

protected:
	size_t dirty_begin_, dirty_end_;
	Event event_;
};

/// Implementation of FileAccessBuf value type.
class FileAccessBuf :
	public CORBA::servant_traits <AccessBuf>::Servant <FileAccessBuf>,
	public FileAccessBufBase
{
	typedef CORBA::servant_traits <AccessBuf>::Servant <FileAccessBuf> Servant;

public:
	FileAccessBuf (const FileSize& position, AccessDirect::_ref_type&& access,
		Bytes&& buffer, uint32_t block_size, uint_fast16_t flags, std::array <char, 2> eol) :
		Servant (position, std::move (access), std::move (buffer), position, block_size, flags, eol)
	{}

	FileAccessBuf (const FileAccessBuf& src);

	// For unmarshal
	FileAccessBuf ()
	{}

	~FileAccessBuf ()
	{
		if (dirty ()) {
			try {
				flush (); //  Last chance
			} catch (...) {}
		}
	}

	void _marshal_in (CORBA::Internal::IORequest::_ptr_type rq) const
	{
		// We can't marshal exclusive locked access object
		if (flags () & O_EXLOCK)
			throw_MARSHAL ();

		// _marshal_in() does not marshal the current buffer
		FileAccessBuf tmp (*this);
		static_cast <const Servant&> (tmp)._marshal_in (rq);
	}

	Nirvana::File::_ref_type file () const
	{
		check ();
		return access ()->file ();
	}

	void close ()
	{
		check ();
		flush_internal ();
		size_t buf_size = buffer ().size ();
		buffer ().clear ();
		buffer ().shrink_to_fit ();
		AccessDirect::_ref_type acc = std::move (access ());
		if (buf_size) {
			LockType lock_type = lock_on_demand ();
			if (LockType::LOCK_NONE != lock_type)
				acc->lock (FileLock (buf_pos (), buf_size, lock_type), FileLock ());
		}
	}

	size_t read (void* p, size_t cb);
	void write (const void* p, size_t cb);
	
	const void* get_buffer_read (size_t& cb)
	{
		check_read ();
		if (flags () & O_TEXT)
			throw_NO_IMPLEMENT (make_minor_errno (ENOSYS));
		if (!cb)
			return nullptr;
		return get_buffer_read_internal (cb);
	}

	void* get_buffer_write (size_t cb);

	void release_buffer (size_t cb)
	{
		check ();
		position (position () + cb);
	}

	FileSize size () const noexcept
	{
		check ();
		return access ()->size ();
	}

	FileSize position () const noexcept
	{
		check ();
		return Servant::position ();
	}

	void position (const FileSize& pos)
	{
		check ();
		Servant::position (pos);
	}

	void flush ()
	{
		check ();
		flush_internal ();
	}

	AccessDirect::_ref_type direct () const noexcept
	{
		return access ();
	}

	uint_fast16_t flags () const noexcept
	{
		return Servant::flags ();
	}

	void flags (uint_fast16_t f)
	{
		check ();
		if (f & O_DIRECT)
			throw_INV_FLAG (make_minor_errno (EINVAL));
		if ((flags () ^ f) & ~(O_APPEND | O_SYNC | O_TEXT))
			throw_INV_FLAG (make_minor_errno (EINVAL));
		Servant::flags (f);
	}

private:
	void check () const;
	void check (const void* p) const;
	void check_read () const;
	void check_write () const;

	LockType lock_on_demand () const noexcept
	{
		return dirty () ?
			((flags () & O_EXLOCK) ? LockType::LOCK_NONE : LockType::LOCK_WRITE)
			:
			((flags () & (O_SHLOCK | O_EXLOCK)) ? LockType::LOCK_NONE : LockType::LOCK_READ);
	}

	void* get_buffer_read_internal (size_t& cb);
	void* get_buffer_write_internal (size_t cb);
	void flush_internal ();
};

inline size_t FileAccessBuf::read (void* p, size_t cb)
{
	check (p);
	check_read ();
	if (!cb)
		return 0;

	if ((flags () & O_TEXT) && eol () [0]) {
		char* dst = (char*)p;
		if (eol () [1]) {
			// 2-char line terminator
			bool prefetch = false;
			if (position () > 0) {
				prefetch = true;
				position (position () - 1);
			}
			size_t cbr = cb * 2;
			const char* src = (const char*)get_buffer_read_internal (cbr);
			if (prefetch)
				position (position () + 1);
			if (cbr) {
				const char* src_end = src + cbr;
				size_t cb_readed = 0;
				if (prefetch) {
					if (cbr >= 2 && src [0] == eol () [0] && src [1] == eol () [1]) {
						src += 2;
						++cb_readed;
					} else
						++src;
				}
				char* end = dst + cb;
				while (src < src_end && dst < end) {
					const char* e = std::find (src, src_end, eol () [0]);
					size_t cc = e - src;
					virtual_copy (src, cc, dst);
					dst += cc;
					cb_readed += cc;
					if (e != src_end) {
						if (e < src_end - 1 && e [1] == eol () [1]) {
							*(dst++) = '\n';
							src = e + 2;
							cb_readed += 2;
						} else {
							*(dst++) = *e;
							src = e + 1;
							++cb_readed;
						}
					}
				}
				position (position () + cb_readed);
			}
			return dst - (char*)p;
		} else {
			const char* src = (const char*)get_buffer_read_internal (cb);
			position (position () + cb);
			const char* end = src + cb;
			while (src < end) {
				const char* e = std::find (src, end, eol () [0]);
				size_t cc = e - src;
				virtual_copy (src, cc, dst);
				dst += cc;
				if (e != end) {
					*(dst++) = '\n';
					src = e + 1;
				} else
					break;
			}
			return cb;
		}
	} else {
		const void* buf = get_buffer_read_internal (cb);
		if (cb) {
			position (position () + cb);
			virtual_copy (buf, cb, p);
		}
		return cb;
	}
}

inline void FileAccessBuf::write (const void* p, size_t cb)
{
	check (p);
	check_write ();

	if (!cb)
		return;

	throw_NO_IMPLEMENT (make_minor_errno (ENOSYS));
}

inline void* FileAccessBuf::get_buffer_write (size_t cb)
{
	check_write ();
	if (!cb)
		return nullptr;

	size_t buf_off = position () % block_size ();
	size_t buf_end = buf_off + cb;
	if (buf_off < buffer ().size ()) {
		if (buf_end > buffer ().size ())
			buffer ().reserve (buf_end);
		return buffer ().data () + buf_off;
	} else {
		flush ();
		FileSize read_pos = buf_pos ();
	}

	throw_NO_IMPLEMENT (make_minor_errno (ENOSYS));
}

}
}

#endif
