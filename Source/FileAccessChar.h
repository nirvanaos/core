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
#ifndef NIRVANA_CORE_FILEACCESSCHAR_H_
#define NIRVANA_CORE_FILEACCESSCHAR_H_
#pragma once

#include "IO_Request.h"
#include "UserAllocator.h"
#include "UserObject.h"
#include <fnctl.h>
#include <Nirvana/SimpleList.h>

namespace Nirvana {
namespace Core {

class FileAccessCharProxy;
class FileChar;

/// Base class for character device access
class FileAccessChar : public UserObject
{
public:
	void write (const IDL::String& data)
	{
		while (write_request_) {
			Ref <IO_Request> rq = write_request_;
			rq->wait ();
			if (rq == write_request_) {
				write_request_ = nullptr;
				break;
			}
		}
		Ref <IO_Request> rq = write_start (data);
		write_request_ = rq;
		rq->wait ();
		if (rq == write_request_)
			write_request_ = nullptr;
		auto err = rq->result ().error;
		if (err)
			throw_INTERNAL (make_minor_errno (err));
	}

	FileChar* file () const noexcept
	{
		return file_;
	}

	void check_flags (unsigned flags) const
	{
		unsigned acc_mode = flags_ & O_ACCMODE;
		if ((acc_mode == O_WRONLY || acc_mode == O_RDONLY) && acc_mode != (flags & O_ACCMODE))
			throw_NO_PERMISSION (make_minor_errno (EACCES)); // File is unaccessible for this mode
	}

	void read_on (FileAccessCharProxy& proxy) noexcept;

	void read_off () noexcept
	{
		if (read_proxies_.empty ())
			read_cancel ();
	}

	int_fast16_t clear_read_error () noexcept
	{
		if (read_error_) {
			int_fast16_t err = read_error_;
			read_error_ = 0;
			if (!read_proxies_.empty ())
				read_start ();
			return err;
		} else
			return 0;
	}

protected:
	template <class> friend class CORBA::servant_reference;

	void _add_ref () noexcept
	{
		ref_cnt_.increment ();
	}

	void _remove_ref () noexcept;

	FileAccessChar (FileChar* file, unsigned flags = O_RDWR, DeadlineTime callback_deadline = 1 * TimeBase::MILLISECOND,
		unsigned initial_buffer_size = 256, unsigned max_buffer_size = 4096);

	virtual ~FileAccessChar ();

	/// Start reading one character.
	/// Must be overridden in the implementation class.
	/// This method must return immediately.
	/// 
	/// When character read, on_read() method will be called.
	/// If error occurred, on_error() method will be called.
	/// 
	/// To stop reading, use read_cancel().
	///
	virtual void read_start () noexcept = 0;

	/// Stops the input wait.
	/// This method must guaratee that after return no on_read() or on_error() callback will be called.
	virtual void read_cancel () noexcept = 0;

	/// Start write buffer to output.
	/// 
	/// \param data Characters to write.
	/// \returns IO_Request reference.
	virtual Ref <IO_Request> write_start (const IDL::String& data) = 0;

	/// Called from the interrupt level when character is readed.
	///  
	/// \param c Readed character.
	void on_read (char c) noexcept;

	/// Called from the interrupt level on reading error.
	///
	/// \param err Error number.
	void on_read_error (int err) noexcept;

private:
	void async_callback () noexcept;
	void read_callback () noexcept;

	static bool is_additional_octet (int c) noexcept
	{
		return (0xC0 & c) == 0x80;
	}

	static unsigned utf8_char_size (int first_octet) noexcept
	{
		if ((first_octet & 0x80) == 0)
			return 1;
		else if ((first_octet & 0xE0) == 0xC0)
			return 2;
		else if ((first_octet & 0xF0) == 0xE0)
			return 3;
		else if ((first_octet & 0xF8) == 0xF0)
			return 4;
		else
			return 0;
	}

	bool push_char (char c, IDL::String& s);
	void push_char (char c, bool& repl, IDL::String& s);

private:
	class ReadCallback : public Runnable
	{
	public:
		ReadCallback (FileAccessChar& obj) :
			object_ (&obj)
		{}

	private:
		virtual void run () override;

	private:
		Ref <FileAccessChar> object_;
	};

private:
	Ref <FileChar> file_;
	std::vector <char, UserAllocator <char> > ring_buffer_;
	unsigned max_buffer_size_;
	unsigned read_pos_;
	unsigned write_pos_;
	char utf8_char_ [4];
	unsigned utf8_char_size_;
	unsigned utf8_octet_cnt_;
	Ref <IO_Request> write_request_;
	Ref <SyncContext> sync_context_;
	RefCounter ref_cnt_;
	AtomicCounter <false> read_available_;
	AtomicCounter <false> write_available_;
	unsigned flags_;
	int read_error_;
	std::atomic_flag callback_;
	DeadlineTime callback_deadline_;
	SimpleList <FileAccessCharProxy> read_proxies_;
};

}
}

#endif
