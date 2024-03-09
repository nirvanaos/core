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
#ifndef NIRVANA_CORE_FILEACCESSCHARPROXY_H_
#define NIRVANA_CORE_FILEACCESSCHARPROXY_H_
#pragma once

#include <CORBA/Server.h>
#include "NameService/FileChar.h"
#include "ORB/ORB.h"
#include <Port/FileSystem.h>

namespace Nirvana {
namespace Core {

class FileAccessCharProxy :
	public CORBA::servant_traits <AccessChar>::Servant <FileAccessCharProxy>,
	public SimpleList <FileAccessCharProxy>::Element
{
public:
	FileAccessCharProxy (FileAccessChar& access, unsigned flags) :
		access_ (&access),
		flags_ (flags),
		pull_consumer_connected_ (false)
	{}

	~FileAccessCharProxy ()
	{
		if (access_) {
			disconnect_push_supplier ();
			disconnect_pull_supplier ();
		}
	}

	Nirvana::File::_ref_type file () const
	{
		check ();
		FileChar* f = access_->file ();
		if (f)
			return f->_this ();
		else
			return nullptr;
	}

	void close ()
	{
		check ();
		disconnect_push_supplier ();
		disconnect_pull_supplier ();
		access_ = nullptr;
	}

	uint_fast16_t flags () const noexcept
	{
		return flags_;
	}

	void set_flags (uint_fast16_t mask, uint_fast16_t f)
	{
		throw_NO_IMPLEMENT ();
	}

	Access::_ref_type dup (uint_fast16_t mask, uint_fast16_t f) const
	{
		check ();
		f = (f & mask) | (flags_ & ~mask);

		return CORBA::make_reference <FileAccessCharProxy> (std::ref (*this))->_this ();
	}

	void write (const IDL::String& data)
	{
		check ();
		access_->write (data);
	}

	int_fast16_t clear_read_error ()
	{
		check ();
		if (flags_ & O_WRONLY)
			throw_NO_PERMISSION (EBADF);
		return access_->clear_read_error ();
	}

	void connect_push_consumer (CosEventComm::PushConsumer::_ptr_type consumer)
	{
		if (push_consumer_)
			throw CosEventChannelAdmin::AlreadyConnected ();
		push_consumer_ = consumer;
	}

	void disconnect_push_supplier () noexcept;

	void connect_pull_consumer (CosEventComm::PullConsumer::_ptr_type consumer)
	{
		if (pull_consumer_connected_)
			throw CosEventChannelAdmin::AlreadyConnected ();
		access_->add_pull_consumer ();
		pull_consumer_connected_ = true;
		pull_consumer_ = consumer;
	}

	void disconnect_pull_supplier () noexcept;

	void push (const CORBA::Any& evt) noexcept
	{
		assert (push_consumer_);
		try {
			if (need_text_fix (evt)) {
				CORBA::Any tmp (evt);
				text_fix (tmp);
				push_consumer_->push (tmp);
			} else
				push_consumer_->push (evt);
		} catch (...) {
			// TODO: Log
		}
	}

	CORBA::Any pull () const
	{
		if (!pull_consumer_connected_)
			throw CosEventComm::Disconnected ();
		CORBA::Any evt = access_->pull ();
		text_fix (evt);
		return evt;
	}

	CORBA::Any try_pull (bool& has_event) const
	{
		if (!pull_consumer_connected_)
			throw CosEventComm::Disconnected ();
		CORBA::Any evt = access_->try_pull (has_event);
		if (has_event)
			text_fix (evt);
		return evt;
	}

private:
	void check () const
	{
		if (!access_)
			throw_BAD_PARAM (make_minor_errno (EBADF));
	}

	static char replace_eol () noexcept
	{
		return Port::FileSystem::eol () [0];
	}

	bool need_text_fix (const CORBA::Any& evt) const
	{
		if (replace_eol ())
			return really_need_text_fix (evt);
		return false;
	}

	bool really_need_text_fix (const CORBA::Any& evt) const;

	static void text_fix (CORBA::Any& evt);

private:
	Ref <FileAccessChar> access_;
	CosEventComm::PushConsumer::_ref_type push_consumer_;
	CosEventComm::PullConsumer::_ref_type pull_consumer_;
	unsigned flags_;
	bool pull_consumer_connected_;
};

}
}
#endif
