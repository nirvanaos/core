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
#include <Nirvana/posix.h>
#include "virtual_copy.h"
#include "Event.h"

namespace Nirvana {
namespace Core {

class FileAccessBufData
{
protected:
	FileAccessBufData () noexcept;

	FileAccessBufData (const FileAccessBufData&) noexcept :
		FileAccessBufData ()
	{}

	FileAccessBufData (FileAccessBufData&&) noexcept :
		FileAccessBufData ()
	{}

	FileAccessBufData& operator = (const FileAccessBufData&) = delete;

	class LockEvent
	{
	public:
		LockEvent (FileAccessBufData& obj) noexcept;

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
	public IDL::traits <AccessBuf>::Servant <FileAccessBuf>,
	public FileAccessBufData
{
	typedef IDL::traits <AccessBuf>::Servant <FileAccessBuf> Servant;

	static const CORBA::UShort ACCESS_COPIED = 0x8000;

public:
	FileAccessBuf (const FileSize& position, const FileSize& buf_pos, AccessDirect::_ref_type&& access,
		Bytes&& buffer, uint32_t block_size, uint_fast16_t flags, std::array <char, 2> eol) :
		Servant (position, buf_pos, std::move (access), std::move (buffer), block_size, flags, eol)
	{}

	// For unmarshal
	FileAccessBuf ()
	{}

	FileAccessBuf (const FileAccessBuf& src) :
		Servant (src),
		FileAccessBufData (src)
	{
		private_flags () |= ACCESS_COPIED;
	}

	~FileAccessBuf ()
	{
		if (dirty ()) {
			try {
				flush (); //  Last chance
			} catch (...) {}
		}
	}

	void _marshal (CORBA::Internal::IORequest_ptr rq)
	{
		Servant::_marshal (rq);
		private_flags () |= ACCESS_COPIED;
	}

	Access::_ref_type dup (uint_fast16_t mask, uint_fast16_t f) const
	{
		mask &= ~ACCESS_COPIED;
		f &= mask;
		f |= flags () & ~mask;
		uint_fast16_t changes = check_flags (f);
		AccessDirect::_ref_type acc;
		if (changes & O_ACCMODE) {
			acc = AccessDirect::_narrow (access ()->dup (O_ACCMODE, f)->_to_object ());
			f &= ~ACCESS_COPIED;
		} else {
			acc = access ();
			f |= ACCESS_COPIED;
		}
		return CORBA::make_reference <FileAccessBuf> (position (), buf_pos (), std::move (acc),
			buffer (), block_size (), f, eol ());
	}

	Nirvana::File::_ref_type file () const
	{
		check ();
		return access ()->file ();
	}

	Nirvana::AccessDirect::_ptr_type direct ();

	void close ()
	{
		check ();
		if (dirty ())
			flush_internal ();
		if (!(private_flags () & ACCESS_COPIED))
			access ()->close ();
		buffer ().clear ();
		buffer ().shrink_to_fit ();
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
		if ((flags () & O_SYNC) && dirty ())
			flush_internal ();
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
		if (dirty ())
			flush_internal (true);
	}

	uint_fast16_t flags () const noexcept
	{
		return Servant::private_flags ();
	}

	void set_flags (uint_fast16_t mask, uint_fast16_t f)
	{
		check ();

		f = (f & mask) | (flags () & ~mask);
		uint_fast16_t changes = check_flags (f);
		if (f & O_DIRECT)
			throw_INV_FLAG (make_minor_errno (EINVAL));

		if (!(flags () & O_SYNC) && (f & O_SYNC) && (flags () & O_ACCMODE)) {
			if (dirty ())
				flush_internal ();
			access ()->flush ();
		}

		if (changes & O_ACCMODE)
			direct ()->set_flags (O_ACCMODE, f);

		Servant::private_flags (f);
	}

private:
	struct BufPos
	{
		FileSize begin;
		FileSize end;
	};

	void check () const;
	void check (const void* p) const;
	void check_read () const;
	void check_write () const;
	uint_fast16_t check_flags (uint_fast16_t f) const;

	const void* get_buffer_read_internal (size_t& cb);
	void* get_buffer_write_internal (size_t cb);
	void flush_internal (bool sync = false);
	void get_new_buf_pos (size_t cb, BufPos& new_buf_pos) const;
	void flush_buf_shift (const BufPos& new_buf_pos);
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
			position (position () + cb);
			return cb;
		}
	} else {
		const void* buf = get_buffer_read_internal (cb);
		if (cb) {
			virtual_copy (buf, cb, p);
			position (position () + cb);
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

	if ((flags () & O_TEXT) && eol () [0]) {

		const char* src = (const char*)p;
		const char* end = src + cb;
		size_t buf_size = eol () [1] ? cb * 2 : cb;
		char* buf;
		Bytes append;
		if (flags () & O_TEXT) {
			append.resize (buf_size);
			buf = (char*)append.data ();
		} else
			buf = (char*)get_buffer_write_internal (buf_size);
		char* dst = buf;
		do {
			const char* e = std::find (src, end, '\n');
			size_t cb = e - src;
			virtual_copy (src, cb, dst);
			dst += cb;
			src += cb;
			if (e != end) {
				(*dst++) = eol () [0];
				if (eol () [1])
					(*dst++) = eol () [1];
				++src;
			}
		} while (src != end);

		size_t cbw = dst - buf;
		if (O_APPEND) {
			append.resize (cbw);
			direct ()->write (std::numeric_limits <FileSize>::max (), append, private_flags () & O_SYNC);
		} else
			position (position () + cbw);

	} else if (flags () & O_APPEND) {
		if (sizeof (size_t) > sizeof (uint32_t) && cb > std::numeric_limits <uint32_t>::max ())
			throw_IMP_LIMIT (make_minor_errno (ENOBUFS));

		direct ()->write (std::numeric_limits <FileSize>::max (), Bytes ((const uint8_t*)p, (const uint8_t*)p + cb), private_flags () & O_SYNC);
		return;
	} else {
		void* buf = get_buffer_write_internal (cb);
		virtual_copy (p, cb, buf);
		position (position () + cb);
	}
	if (flags () & O_SYNC)
		flush_internal ();
}

inline void* FileAccessBuf::get_buffer_write (size_t cb)
{
	check_write ();
	if (flags () & (O_TEXT | O_APPEND))
		throw_NO_IMPLEMENT (make_minor_errno (ENOSYS));
	if (!cb)
		return nullptr;
	return get_buffer_write_internal (cb);
}

}
}

#endif
