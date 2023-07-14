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
#include <virtual_copy.h>
#include <Nirvana/bitutils.h>
#include <fnctl.h>

namespace Nirvana {
namespace Core {

class FileAccessBuf : public CORBA::servant_traits <AccessBuf>::Servant <FileAccessBuf>
{
	typedef CORBA::servant_traits <AccessBuf>::Servant <FileAccessBuf> Base;

public:
	FileAccessBuf (Bytes&& data, AccessDirect::_ref_type&& access, FileSize pos, uint32_t block_size,
		uint_fast16_t flags, std::array <char, 2> eol) :
		Base (std::move (data), std::move (access), pos, block_size, flags, eol),
		dirty_begin_ (std::numeric_limits <size_t>::max ()),
		dirty_end_ (0)
	{}

	FileAccessBuf () :
		dirty_begin_ (std::numeric_limits <size_t>::max ()),
		dirty_end_ (0)
	{}

	~FileAccessBuf ()
	{
		if (dirty ()) {
			try {
				flush (); //  Last chance
			} catch (...) {}
		}
	}

	Nirvana::File::_ref_type file () const
	{
		check ();
		return access ()->file ();
	}

	void close ()
	{
		check ();
		flush ();
		buffer ().clear ();
		buffer ().shrink_to_fit ();
		access ()->close ();
		access () = nullptr;
	}

	size_t read (void* p, size_t cb)
	{
		check (p);
		check_read ();

		uint8_t* dst = (uint8_t*)p;
		uint8_t* end = dst + cb;

		if (dst < end) {
			// Read from current buffer
			dst = read_from_buffer (dst, end);

			if (dst < end) {
				// Read next blocks
				if (sizeof (size_t) > sizeof (uint32_t)) {
					while (dst < end) {
						size_t chunk = end - dst;
						if (chunk > std::numeric_limits <uint32_t>::max ())
							chunk = round_down (std::numeric_limits <uint32_t>::max (), block_size ());
						read_next_buffer ((uint32_t)chunk);
						dst = read_from_buffer (dst, end);
					}
				} else {
					read_next_buffer ((uint32_t)(end - dst));
					dst = read_from_buffer (dst, end);
				}
			}
		}

		// Release excessive memory
		// Resulting buffer size will be <= block_size
		if (buffer ().size () > block_size ()) {
			size_t release_size = round_down (buffer ().size () - 1, (size_t)block_size ());
			buffer ().erase (buffer ().begin (), buffer ().begin () + release_size);
		}

		return dst - (uint8_t*)p;
	}

	void write (const void* p, size_t cb)
	{
		check (p);
		check_write ();

		if (!cb)
			return;

		const uint8_t* src = (const uint8_t*)p;
		const uint8_t* end = src + cb;

		size_t buf_off = position () % block_size ();
		if (buf_off < buffer ().size ()) {

		}

		if (buf_off >= buffer ().size ()) {
			flush ();
			if (cb >= block_size ()) {
				FileSize next = position () + buffer ().size () - buf_off;
				assert (next % block_size () == 0);
				size_t tail = cb % block_size ();
				if (!tail) {
					buffer ().clear ();
					buffer ().shrink_to_fit ();
				}
				size_t full_blocks = cb - tail;
				Bytes tmp (src, src + full_blocks);
				access ()->write (next, tmp);
				position (next + full_blocks);
				cb -= full_blocks;
				p = (const uint8_t*)p + full_blocks;
			}
			if (cb) {
				assert (cb < block_size ());
				if ((flags () & O_ACCMODE) != O_WRONLY)
					read_next_buffer (block_size ());
				buffer ().reserve (block_size ());
			}
			buf_off = 0;
		}
		if (cb) {
			// Write tail to buffer
			assert (cb < block_size ());
			assert (buf_off < buffer ().capacity ());
			size_t chunk_end = buf_off + cb;
			if (buffer ().size () > buf_off) {
				size_t copy_size = buffer ().size () - buf_off;
				if (copy_size > cb) {
					copy_size = cb;
					virtual_copy (p, copy_size, buffer ().data () + buf_off);
					p = (const uint8_t*)p + copy_size;
				} else
					buffer ().resize (buf_off);
			}
			if (chunk_end > buffer ().size ()) {
				size_t append_size = chunk_end - buffer ().size ();
				buffer ().insert (buffer ().end (), (const uint8_t*)p, (const uint8_t*)p + append_size);
				// Not need: p = (const uint8_t*)p + append_size;
			}
			if (dirty_end_ < chunk_end)
				dirty_end_ = chunk_end;
			if (dirty_begin_ > buf_off)
				dirty_begin_ = buf_off;
			position () += cb;
		}
	}

	void* get_buffer_write (size_t cb)
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
			FileSize read_pos = buffer_position ();
		}

		throw CORBA::NO_IMPLEMENT ();
	}

	const void* get_buffer_read (size_t cb)
	{
		check_read ();
		throw CORBA::NO_IMPLEMENT ();
	}

	void release_buffer (size_t cb)
	{
		check ();
		set_position (position () + cb);
	}

	FileSize pos () const noexcept
	{
		check ();
		return Base::position ();
	}

	FileSize size () const noexcept
	{
		check ();
		return access ()->size ();
	}

	FileSize seek (SeekMethod m, FileOff off)
	{
		check ();
		FileSize pos = position ();
		switch (m) {
		case SeekMethod::SM_END:
			pos = access ()->size ();
#ifdef NIRVANA_C17
			[[fallthrough]];
#endif
		case SeekMethod::SM_CUR:
			if (off < 0) {
				if (pos + off < 0)
					throw RuntimeError (EOVERFLOW);
			}
			pos += off;
			break;

		default:
			if (off < 0)
				throw RuntimeError (EOVERFLOW);
			pos = off;
		}

		set_position (pos);
		return pos;
	}

	void flush ();

	bool lock (FileLock& fl, short op)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	AccessDirect::_ref_type direct () const noexcept
	{
		return access ();
	}

	uint_fast16_t flags () const noexcept
	{
		return Base::flags ();
	}

	void flags (uint_fast16_t f)
	{
		if (f & O_DIRECT)
			throw CORBA::INV_FLAG (make_minor_errno (EINVAL));
		check ();
		access ()->flags (f);
		Base::flags (f);
	}

	Access::_ref_type dup ()
	{
		check ();
		AccessDirect::_ref_type acc = AccessDirect::_narrow (access ()->dup ()->_to_object ());
		return CORBA::make_reference <FileAccessBuf> (Bytes (buffer ()), std::move (acc), pos (),
			block_size (), flags (), eol ());
	}

protected:
	FileSize buffer_position () const noexcept
	{
		return round_down (position (), (FileSize)block_size ());
	}

	void set_position (FileSize pos);

	void check () const;
	void check (const void* p) const;
	void check_read () const;
	void check_write () const;
	void read_next_buffer (uint32_t cb);
	uint8_t* read_from_buffer (uint8_t* dst, uint8_t* end);

	bool dirty () const noexcept
	{
		return dirty_begin_ < dirty_end_;
	}

private:
	size_t dirty_begin_, dirty_end_;
};

}
}

#endif
