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
		while (write_request_)
			write_request_->wait ();
		write_request_ = write_start (data);
		write_request_->wait ();
		auto err = write_request_->result ().error;
		write_request_ = nullptr;
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

	void read_on () noexcept
	{
		if (1 == ++read_proxy_cnt_)
			read_start ();
	}

	void read_off () noexcept
	{
		if (0 == --read_proxy_cnt_)
			read_cancel ();
	}

protected:
	template <class> friend class CORBA::servant_reference;

	void _add_ref () noexcept
	{
		ref_cnt_.increment ();
	}

	void _remove_ref () noexcept;

	FileAccessChar (FileChar* file, unsigned flags = O_RDWR, DeadlineTime callback_deadline = 1 * TimeBase::MILLISECOND,
		unsigned initial_buffer_size = 256);

	virtual ~FileAccessChar ();

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
	void async_callback (int err) noexcept;
	void read_callback (int err) noexcept;
	static const char* get_valid_utf8_end (const char* begin, const char* end);
	
private:
	class ReadCallback : public Runnable
	{
	public:
		ReadCallback (FileAccessChar& obj, int err) :
			object_ (&obj),
			error_ (err)
		{}

	private:
		virtual void run () override;

	private:
		Ref <FileAccessChar> object_;
		int error_;
	};

	Ref <FileChar> file_;
	std::vector <char, UserAllocator <char> > ring_buffer_;
	unsigned read_pos_;
	unsigned write_pos_;
	char utf8_tail_ [3];
	unsigned utf8_tail_size_;
	Ref <IO_Request> write_request_;
	Ref <SyncContext> sync_context_;
	RefCounter ref_cnt_;
	AtomicCounter <false> read_available_;
	AtomicCounter <false> write_available_;
	unsigned flags_;
	int read_error_;
	std::atomic_flag callback_;
	DeadlineTime callback_deadline_;
	SimpleList <FileAccessCharProxy> proxies_;
	unsigned read_proxy_cnt_;
};

}
}

#endif
