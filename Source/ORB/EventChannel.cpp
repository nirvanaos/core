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

void EventChannelBase::ProxyPushConsumer::disconnect_push_consumer () noexcept
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

void EventChannelBase::ProxyPullSupplier::disconnect_pull_supplier () noexcept
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
		event_.signal ();
	}
}

void EventChannelBase::ProxyPullConsumer::disconnect_pull_consumer () noexcept
{
	CosEventComm::PullSupplier::_ref_type p = std::move (supplier_);
	if (p) {
		handler_->disconnect ();
		handler_ = nullptr;
		channel_->pull_consumers_.on_disconnect ();
		try {
			p->disconnect_pull_supplier ();
		} catch (...) {}
	}
}

void EventChannelBase::ProxyPushSupplier::disconnect_push_supplier () noexcept
{
	CosEventComm::PushConsumer::_ref_type p = std::move (consumer_);
	if (p) {
		channel_->push_suppliers_.on_disconnect ();
		try {
			p->disconnect_push_consumer ();
		} catch (...) {}
	}
}

void EventChannelBase::Children::destroy (PortableServer::POA::_ptr_type adapter) noexcept
{
	for (auto p : *this) {
		p->destroy (adapter);
	}
	clear ();
}

void EventChannelBase::push (const Any& data)
{
	if (!pull_suppliers_.empty ()) {
		servant_reference <SharedAny> sany (make_reference <SharedAny> (std::ref (data)));
		for (const auto p : pull_suppliers_) {
			ProxyPullSupplier& sup = static_cast <ProxyPullSupplier&> (*p);
			if (sup.connected ())
				sup.push (*sany);
		}
	}

	for (const auto p : push_suppliers_) {
		ProxyPushSupplier& sup = static_cast <ProxyPushSupplier&> (*p);
		if (sup.connected ())
			sup.push (data);
	}
}

void EventChannelBase::deactivate (PortableServer::ServantBase::_ptr_type servant,
	PortableServer::POA::_ptr_type adapter) noexcept
{
	try {
		adapter->deactivate_object (adapter->servant_to_id (servant));
	} catch (...) {
	}
}

}
}
