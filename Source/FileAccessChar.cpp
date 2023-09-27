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
#include "FileAccessCharProxy.h"
#include "NameService/FileChar.h"
#include "Chrono.h"

namespace Nirvana {
namespace Core {

FileAccessChar::FileAccessChar (FileChar* file, unsigned flags, DeadlineTime callback_deadline, unsigned initial_buffer_size) :
	file_ (file),
	ring_buffer_ (initial_buffer_size),
	read_pos_ (0),
	write_pos_ (0),
	utf8_tail_size_ (0),
	sync_context_ (&SyncContext::current ()),
	read_available_ (0),
	write_available_ (initial_buffer_size),
	flags_ (flags),
	callback_ ATOMIC_FLAG_INIT,
	callback_deadline_ (callback_deadline),
	read_error_ (0),
	read_proxy_cnt_ (0)
{}

FileAccessChar::~FileAccessChar ()
{}

void FileAccessChar::_remove_ref () noexcept
{
	if (0 == ref_cnt_.decrement ()) {
		read_cancel ();
		if (write_request_) {
			write_request_->cancel ();
			write_request_->wait ();
		}
		if (file_)
			file_->on_access_destroy ();
		delete this;
	}
}

void FileAccessChar::on_read (char c) noexcept
{
	ring_buffer_ [write_pos_++] = c;
	read_available_.increment ();
	if (write_available_.decrement_seq ())
		read_start ();
	async_callback (0);
}

void FileAccessChar::on_read_error (int err) noexcept
{
	async_callback (err);
}

void FileAccessChar::async_callback (int err) noexcept
{
	if (!callback_.test_and_set () || err) {
		try {
			ExecDomain::async_call <ReadCallback> (Chrono::make_deadline (callback_deadline_), *sync_context_,
				nullptr, std::ref (*this), err);
		} catch (...) {
			callback_.clear ();
		}
	}
}

inline
void FileAccessChar::read_callback (int err) noexcept
{
	callback_.clear ();
	auto cc = read_available_.load ();
	assert (cc || err);
	CharFileEvent evt;
	evt.error ((int16_t)err);
	if (cc) {
		bool overflow = cc == ring_buffer_.size ();
		try {
			
			unsigned tail = utf8_tail_size_;
			utf8_tail_size_ = 0;
			evt.data ().reserve (cc + tail);
			evt.data ().append (utf8_tail_, tail);

			while (cc--) {
				evt.data ().push_back (ring_buffer_ [read_pos_]);
				read_pos_ = (read_pos_ + 1) % ring_buffer_.size ();
				read_available_.decrement ();
				write_available_.increment ();
			}
		} catch (...) {
			evt.error (ENOMEM);
		}

		if (overflow) {
			try {
				ring_buffer_.resize (ring_buffer_.size () * 2);
			} catch (...) {
				// TODO: Log
			}
			read_start ();
		}

		const char* end = evt.data ().data () + evt.data ().size ();
		const char* utf8_end = end;
		try {
			utf8_end = get_valid_utf8_end (evt.data ().data (), end);
		} catch (...) {
			evt.data ().clear ();
			evt.error (EILSEQ);
		}
		if (utf8_end < end) {
			std::copy (utf8_end, end, utf8_tail_);
			utf8_tail_size_ = (unsigned)(end - utf8_end);
			evt.data ().resize (utf8_end - evt.data ().data ());
		}
	}

	for (auto it = proxies_.begin (); it != proxies_.end (); ++it) {
		it->push (evt);
	}
}

void FileAccessChar::ReadCallback::run ()
{
	object_->read_callback (error_);
}

const char* FileAccessChar::get_valid_utf8_end (const char* begin, const char* end)
{
	if (begin < end) {
		// Drop incomplete UTF8 data
		if ((0xC0 & *(end - 1)) == 0x80) {
			const char* pfirst = end - 1;
			size_t additional_octets = 1;
			while (pfirst > begin && additional_octets < 3) {
				if ((0xC0 & *(pfirst - 1)) == 0x80) {
					++additional_octets;
					--pfirst;
				}
			}
			char first_octet = *(--pfirst);
			size_t additional_octets_expected;
			if ((first_octet & 0x80) == 0)
				additional_octets_expected = 0;
			else if ((first_octet & 0xE0) == 0xC0)
				additional_octets_expected = 1;
			else if ((first_octet & 0xF0) == 0xE0)
				additional_octets_expected = 2;
			else if ((first_octet & 0xF8) == 0xF0)
				additional_octets_expected = 3;
			else
				throw_CODESET_INCOMPATIBLE ();

			if (additional_octets_expected > additional_octets)
				return pfirst;
			else if (additional_octets_expected < additional_octets)
				throw_CODESET_INCOMPATIBLE ();
		}
	}
	return end;
}

}
}
