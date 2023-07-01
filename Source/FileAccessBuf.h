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
	FileAccessBuf (Bytes&& data, AccessDirect::_ref_type&& access, FileSize pos, uint32_t block_size, unsigned flags) :
		Base (std::move (data), std::move (access), pos, block_size, flags),
		dirty_begin_ (std::numeric_limits <size_t>::max ()),
		dirty_end_ (0)
	{}

	FileAccessBuf () :
		dirty_begin_ (std::numeric_limits <size_t>::max ()),
		dirty_end_ (0)
	{}

	~FileAccessBuf ()
	{}

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
		// Do not close acces, other copies may be active.
		// access ()->close ();
		access () = nullptr;
	}

	size_t read (void* p, size_t cb)
	{
		check (p);
		check_read ();

		size_t readed = 0;
		while (cb) {
			size_t buf_off = position () % block_size ();
			if (buf_off < buffer ().size ()) {
				size_t chunk_size = buffer ().size () - buf_off;
				if (chunk_size > cb)
					chunk_size = cb;
				virtual_copy (buffer ().data () + buf_off, chunk_size, p);
				cb -= chunk_size;
				readed += chunk_size;
				p = (uint8_t*)p + chunk_size;
				position () += chunk_size;
			}
			if (cb) {
				flush ();
				read_next_buffer ();
				if (buffer ().empty ())
					break;
			}
		}
		return readed;
	}

	void write (const void* p, size_t cb)
	{
		check (p);
		check_write ();

		size_t buf_off = position () % block_size ();
		if (buf_off >= buffer ().size ()) {
			flush ();
			if (cb >= block_size ()) {
				FileSize next = position () - buf_off + buffer ().size ();
				assert (next % block_size () == 0);
				size_t tail = cb % block_size ();
				if (!tail) {
					buffer ().clear ();
					buffer ().shrink_to_fit ();
				}
				size_t full_blocks = cb - tail;
				Bytes tmp ((const uint8_t*)p, (const uint8_t*)p + full_blocks);
				access ()->write (next, tmp);
				position (next + full_blocks);
				cb -= full_blocks;
				p = (const uint8_t*)p + full_blocks;
			}
			if (cb) {
				assert (cb < block_size ());
				if ((flags () & O_ACCMODE) != O_WRONLY)
					read_next_buffer ();
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

	const void* get_buffer_read (size_t cb)
	{
		check_read ();
		throw CORBA::NO_IMPLEMENT ();
	}

	void* get_buffer_write (size_t cb)
	{
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
	void read_next_buffer ();

private:
	size_t dirty_begin_, dirty_end_;
};

}
}

#endif
