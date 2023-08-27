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
#include "EventChannel.h"

namespace CORBA {
namespace Core {

void EventChannel::ProxyPushConsumer::disconnect_push_consumer () noexcept
{
	if (connected_) {
		CosEventComm::PushSupplier::_ref_type p = std::move (supplier_);
		connected_ = false;
		channel_->push_consumers_.on_disconnect ();
		if (p) {
			try {
				p->disconnect_push_supplier ();
			} catch (...) {}
		}
	}
}

void EventChannel::ProxyPullSupplier::disconnect_pull_supplier () noexcept
{
	if (connected_) {
		CosEventComm::PullConsumer::_ref_type p = std::move (consumer_);
		connected_ = false;
		channel_->pull_suppliers_.on_disconnect ();
		if (p) {
			try {
				p->disconnect_pull_consumer ();
			} catch (...) {}
		}
	}
}

void EventChannel::ProxyPullConsumer::disconnect_pull_consumer () noexcept
{
	CosEventComm::PullSupplier::_ref_type p = std::move (supplier_);
	if (p) {
		channel_->pull_consumers_.on_disconnect ();
		try {
			p->disconnect_pull_supplier ();
		} catch (...) {}
	}
}

void EventChannel::ProxyPushSupplier::disconnect_push_supplier () noexcept
{
	CosEventComm::PushConsumer::_ref_type p = std::move (consumer_);
	if (p) {
		channel_->push_suppliers_.on_disconnect ();
		try {
			p->disconnect_push_consumer ();
		} catch (...) {}
	}
}

}
}
