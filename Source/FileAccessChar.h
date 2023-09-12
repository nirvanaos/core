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

	void read (uint32_t size, IDL::String& data);

protected:
	template <class> friend class CORBA::servant_reference;

	void _add_ref () noexcept
	{
		ref_cnt_.increment ();
	}

	void _remove_ref () noexcept
	{
		if (0 == ref_cnt_.decrement ()) {
			read_cancel ();
			if (write_request_) {
				write_request_->cancel ();
				write_request_->wait ();
			}
			delete this;
		}
	}

	FileAccessChar (DeadlineTime callback_deadline = 1 * TimeBase::MILLISECOND,
		unsigned initial_buffer_size = 256);

	virtual ~FileAccessChar ()
	{}

	virtual void read_start () = 0;
	virtual void read_cancel () = 0;
	void on_read (char c) noexcept;

	virtual Ref <IO_Request> write_start (const IDL::String& data) = 0;

private:
	void read_callback ();

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

	std::vector <char, UserAllocator <char> > buffer_;
	std::vector <char, UserAllocator <char> > ring_buffer_;
	Ref <IO_Request> write_request_;
	Ref <SyncContext> sync_context_;
	size_t read_pos_;
	size_t write_pos_;
	RefCounter ref_cnt_;
	AtomicCounter <false> read_available_;
	AtomicCounter <false> write_available_;
	std::atomic_flag callback_;
	DeadlineTime callback_deadline_;
};

}
}

#endif
