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
#include "FileAccessChar.h"
#include "Chrono.h"

namespace Nirvana {
namespace Core {

FileAccessChar::FileAccessChar (DeadlineTime callback_deadline, unsigned initial_buffer_size) :
	ring_buffer_ (initial_buffer_size),
	sync_context_ (&SyncContext::current ()),
	read_pos_ (0),
	write_pos_ (0),
	read_available_ (0),
	write_available_ (initial_buffer_size),
	callback_ ATOMIC_FLAG_INIT,
	callback_deadline_ (callback_deadline)
{
	buffer_.reserve (initial_buffer_size);
}

void FileAccessChar::on_read (char c) noexcept
{
	ring_buffer_ [write_pos_++] = c;
	read_available_.increment ();
	if (write_available_.decrement_seq ())
		read_start ();
	if (!callback_.test_and_set ()) {
		try {
			ExecDomain::async_call <Read> (Chrono::make_deadline (callback_deadline_), *sync_context_,
				nullptr, std::ref (*this));
		} catch (...) {
			callback_.clear ();
		}
	}
}

inline
void FileAccessChar::read_callback ()
{
	callback_.clear ();
	auto cc = read_available_.load ();
}

void FileAccessChar::Read::run ()
{
	object_->read_callback ();
}

}
}
