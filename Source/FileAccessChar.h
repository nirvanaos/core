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
#include <Nirvana/File.h>
#include "EventSync.h"

namespace Nirvana {
namespace Core {

/// Base class for character device access
class FileAccessChar : public UserObject
{
public:
	void write (const IDL::String& data)
	{
		while (write_request_)
			write_request_->wait ();
		write_request_ = write_start (data);
		write_request_->wait ();
		auto err = write_request_->result ().error;
		write_request_ = nullptr;
		if (err)
			throw_INTERNAL (make_minor_errno (err));
	}

	void read (uint32_t size, IDL::String& data, bool wait)
	{
		if (!size)
			return;

		for (;;) {
			size_t cc = std::min (buffer_.size (), (size_t)size);
			if (cc) {
				try {
					cc = get_valid_utf8_end (buffer_.data (), buffer_.data () + cc) - buffer_.data ();
				} catch (...) {
					buffer_.clear ();
					read_event_.reset ();
					throw; // CODESET_INCOMPATIBLE
				}
			}

			if (cc) {
				data.assign (buffer_.data (), cc);
				buffer_.erase (buffer_.begin (), buffer_.begin () + cc);
				if (buffer_.empty ())
					read_event_.reset ();
				break;
			} else if (exception_code_ != CORBA::Exception::EC_NO_EXCEPTION) {
				CORBA::Exception::Code ec = exception_code_;
				exception_code_ = CORBA::Exception::EC_NO_EXCEPTION;
				CORBA::SystemException::_raise_by_code (ec, exception_data_.minor, exception_data_.completed);
			} else if (wait)
				read_event_.wait ();
			else
				break;
		}
	}

	Nirvana::File::_ref_type file () const noexcept
	{
		return file_;
	}

protected:
	template <class> friend class CORBA::servant_reference;

	void _add_ref () noexcept
	{
		ref_cnt_.increment ();
	}

	void _remove_ref () noexcept;

	FileAccessChar (Nirvana::File::_ptr_type file, unsigned flags = O_RDWR, DeadlineTime callback_deadline = 1 * TimeBase::MILLISECOND,
		unsigned initial_buffer_size = 256);

	virtual ~FileAccessChar ()
	{}

	virtual void read_start () noexcept = 0;
	virtual void read_cancel () noexcept = 0;
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
	void read_callback ();
	static const char* get_valid_utf8_end (const char* begin, const char* end);
	
	void on_exception (const CORBA::SystemException& ex) noexcept
	{
		exception_code_ = ex.__code ();
		exception_data_ = ex._data ();
	}

private:
	class Read : public Runnable
	{
	public:
		Read (FileAccessChar& obj) :
			object_ (&obj)
		{}

	private:
		virtual void run () override;

	private:
		Ref <FileAccessChar> object_;
	};

	Nirvana::File::_ref_type file_;
	std::vector <char, UserAllocator <char> > buffer_;
	std::vector <char, UserAllocator <char> > ring_buffer_;
	Ref <IO_Request> write_request_;
	Ref <SyncContext> sync_context_;
	size_t read_pos_;
	size_t write_pos_;
	RefCounter ref_cnt_;
	AtomicCounter <false> read_available_;
	AtomicCounter <false> write_available_;
	unsigned flags_;
	std::atomic_flag callback_;
	DeadlineTime callback_deadline_;
	EventSync read_event_;
	CORBA::Exception::Code exception_code_;
	CORBA::SystemException::_Data exception_data_;
};

}
}

#endif
