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
#include <Nirvana/File_s.h>
#include "EventSync.h"
#include <queue>

namespace Nirvana {
namespace Core {

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

	void received (const CharFileEvent& evt)
	{
		if (!evt.error () && !queue_.empty () && !queue_.back ().error ())
			queue_.back ().data () += evt.data ();
		else {
			queue_.push (evt);
			event_.signal ();
		}
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
			CosEventChannelAdmin::ProxyPushSupplier::_ref_type sup = std::move (supplier_);
			sup->disconnect_push_supplier ();
		}
	}

	size_t read (void* p, size_t size)
	{
		if (!size)
			return 0;

		while (queue_.empty ()) {
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

		uint8_t* dst = (uint8_t*)p;
		for (;;) {
			const CharFileEvent& front = queue_.front ();
			assert (!front.error ());
			size_t cbr = std::min (size, front.data ().size ());
			virtual_copy (front.data ().data (), cbr, dst);
			dst += cbr;
			size -= cbr;
			if (front.data ().size () == cbr) {
				queue_.pop ();
				if (queue_.empty () || queue_.front ().error ())
					break;
			} else
				break;
		}
		return dst - (uint8_t*)p;
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
