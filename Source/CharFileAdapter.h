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
#ifndef NIRVANA_CORE_CHARFILEADAPTER_H_
#define NIRVANA_CORE_CHARFILEADAPTER_H_
#pragma once

#include <CORBA/Server.h>
#include <Nirvana/File_s.h>
#include <Nirvana/real_copy.h>
#include "EventSync.h"
#include <queue>

namespace Nirvana {
namespace Core {

/// \brief Adapter to implement POSIX read() over the character device
class CharFileAdapter :
	public CORBA::servant_traits <CharFileReceiver>::Servant <CharFileAdapter>
{
public:
	CharFileAdapter (AccessChar::_ptr_type file, bool non_block) :
		non_block_ (non_block)
	{
		CosEventChannelAdmin::ProxyPushSupplier::_ref_type sup = file->for_consumers ()->obtain_push_supplier ();
		sup->connect_push_consumer (_this ());
		supplier_ = std::move (sup);
	}

	void received (CharFileEvent& evt)
	{
		if (evt.error () && !queue_.empty () && queue_.back ().error () == evt.error ())
			return; // Prevent infinite error collecting
		queue_.push (std::move (evt));
		event_.signal ();
	}

	CORBA::Object::_ref_type get_typed_consumer ()
	{
		return _this ();
	}

	void push (const CORBA::Any& data)
	{}

	void disconnect_push_consumer ()
	{
		if (supplier_) {
			CosEventChannelAdmin::ProxyPushSupplier::_ref_type sup (std::move (supplier_));
			event_.signal ();
			sup->disconnect_push_supplier ();
		}
	}

	size_t read (void* p, size_t size)
	{
		if (!size)
			return 0;

		while (queue_.empty ()) {
			if (!supplier_)
				throw_INTERNAL (make_minor_errno (EBADF)); // Closed, disconnected

			if (non_block_)
				throw_INTERNAL (make_minor_errno (EAGAIN));
			else
				event_.wait ();
		}

		int err = queue_.front ().error ();
		if (err) {
			queue_.pop ();
			throw_INTERNAL (make_minor_errno (err));
		}

		char* dst = (char*)p;
		for (;;) {
			CharFileEvent& front = queue_.front ();
			assert (!front.error ());
			size_t cbr = std::min (size, front.data ().size ());
			real_copy (front.data ().data (), front.data ().data () + cbr, dst);
			dst += cbr;
			size -= cbr;
			if (front.data ().size () == cbr) {
				queue_.pop ();
				if (queue_.empty () || queue_.front ().error ())
					break;
			} else {
				front.data ().erase (0, cbr);
				break;
			}
		}
		return dst - (char*)p;
	}

	bool non_block () const noexcept
	{
		return non_block_;
	}

	void non_block (bool nb) noexcept
	{
		non_block_ = nb;
	}

private:
	CosEventChannelAdmin::ProxyPushSupplier::_ref_type supplier_;
	std::queue <CharFileEvent> queue_;
	EventSync event_;
	bool non_block_;
};

}
}

#endif
