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

namespace Nirvana {
namespace Core {

class FileAccessCharProxy :
	public CORBA::servant_traits <AccessChar>::Servant <FileAccessCharProxy>,
	public SimpleList <FileAccessCharProxy>::Element
{
public:
	FileAccessCharProxy (FileAccessChar& access, unsigned flags) :
		access_ (&access),
		flags_ (flags)
	{}

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
		access_ = nullptr;
	}

	uint_fast16_t flags () const noexcept
	{
		return flags_;
	}

	void flags (unsigned fl);

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

	CosTypedEventChannelAdmin::TypedConsumerAdmin::_ref_type for_consumers ()
	{
		check ();
		if (flags_ & O_WRONLY)
			throw_NO_PERMISSION (EBADF);
		if (!event_channel_) {
			event_channel_ = CORBA::Core::ORB::create_typed_channel ();
			CosTypedEventChannelAdmin::TypedProxyPushConsumer::_ref_type consumer =
				event_channel_->for_suppliers ()->obtain_typed_push_consumer (CORBA::Internal::RepIdOf <CharFileSink>::id);
			sink_ = CharFileSink::_narrow (consumer->get_typed_consumer ());
			assert (sink_);
		}
		return event_channel_->for_consumers ();
	}

	void push (const CharFileEvent& evt) noexcept
	{
		if (sink_) {
			try {
				sink_->received (evt);
			} catch (...) {
				// TODO: Log
			}
		}
	}

private:
	void check () const
	{
		if (!access_)
			throw_BAD_PARAM (make_minor_errno (EBADF));
	}

private:
	Ref <FileAccessChar> access_;
	CosTypedEventChannelAdmin::TypedEventChannel::_ref_type event_channel_;
	CharFileSink::_ref_type sink_;
	unsigned flags_;
};

}
}
#endif
