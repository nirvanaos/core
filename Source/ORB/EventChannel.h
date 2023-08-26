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
#include <CORBA/Server.h>
#include "../CORBA/CosEventChannelAdmin_s.h"
#include "../CoreInterface.h"
#include "../EventSync.h"
#include <queue>

namespace CORBA {
namespace Core {

typedef Nirvana::Core::ImplDynamicSync <Any> SharedAny;

class EventChannel :
	public servant_traits <CosEventChannelAdmin::EventChannel>::Servant <EventChannel>
{
public:
	CosEventChannelAdmin::ConsumerAdmin::_ref_type for_consumers ()
	{
		if (!consumer_admin_)
			consumer_admin_ = make_reference <ConsumerAdmin> (std::ref (*this));
		return consumer_admin_->_this ();
	}

	CosEventChannelAdmin::SupplierAdmin::_ref_type for_suppliers ()
	{
		if (!supplier_admin_)
			supplier_admin_ = make_reference <SupplierAdmin> (std::ref (*this));
		return supplier_admin_->_this ();
	}

	void destroy ()
	{}

private:
	void push (const Any& data)
	{}

	class ProxyPushConsumer :
		public servant_traits <CosEventChannelAdmin::ProxyPushConsumer>::Servant <ProxyPushConsumer>
	{
	public:
		ProxyPushConsumer (EventChannel& channel) :
			channel_ (channel),
			connected_ (false)
		{}

		void connect_push_supplier (CosEventComm::PushSupplier::_ptr_type push_supplier)
		{
			if (connected_)
				throw CosEventChannelAdmin::AlreadyConnected ();
			supplier_ = push_supplier;
		}

		void disconnect_push_consumer ()
		{
			CosEventComm::PushSupplier::_ref_type p = std::move (supplier_);
			connected_ = false;
			if (p)
				p->disconnect_push_supplier ();
		}

		void push (const Any& data)
		{
			if (!connected_)
				throw CosEventComm::Disconnected ();
			channel_.push (data);
		}

	private:
		EventChannel& channel_;
		CosEventComm::PushSupplier::_ref_type supplier_;
		bool connected_;
	};

	class ProxyPullSupplier :
		public servant_traits <CosEventChannelAdmin::ProxyPullSupplier>::Servant <ProxyPullSupplier>
	{
	public:
		ProxyPullSupplier (EventChannel& channel) :
			channel_ (channel),
			connected_ (false)
		{}

		void connect_pull_consumer (CosEventComm::PullConsumer::_ptr_type pull_consumer)
		{
			if (connected_)
				throw CosEventChannelAdmin::AlreadyConnected ();
			consumer_ = pull_consumer;
		}

		void disconnect_pull_supplier ()
		{
			CosEventComm::PullConsumer::_ref_type p = std::move (consumer_);
			connected_ = false;
			if (p)
				p->disconnect_pull_consumer ();
		}

		Any pull ()
		{
			if (!connected_)
				throw CosEventComm::Disconnected ();
			if (queue_.empty ())
				channel_.event_.wait ();
			return pull_internal ();
		}

		Any try_pull (bool& has_event)
		{
			if (!connected_)
				throw CosEventComm::Disconnected ();
			if (queue_.empty ()) {
				has_event = false;
				return Any ();
			}
			has_event = true;
			return pull_internal ();
		}

		void push (SharedAny& data)
		{
			queue_.push (&data);
		}

	private:
		Any pull_internal () noexcept
		{
			servant_reference <SharedAny> ref (std::move (queue_.front ()));
			queue_.pop ();
			return Any (std::move (*ref));
		}

	private:
		EventChannel& channel_;
		CosEventComm::PullConsumer::_ref_type consumer_;
		std::queue <servant_reference <SharedAny> > queue_;
		bool connected_;
	};

	class ConsumerAdmin :
		public servant_traits <CosEventChannelAdmin::ConsumerAdmin>::Servant <ConsumerAdmin>
	{
	public:
		ConsumerAdmin (EventChannel& channel) :
			channel_ (channel)
		{}

		CosEventChannelAdmin::ProxyPushSupplier::_ref_type obtain_push_supplier ()
		{
			return nullptr;
		}

		CosEventChannelAdmin::ProxyPullSupplier::_ref_type obtain_pull_supplier ()
		{
			return nullptr;
		}

	private:
		EventChannel& channel_;
	};

	class SupplierAdmin :
		public servant_traits <CosEventChannelAdmin::SupplierAdmin>::Servant <SupplierAdmin>
	{
	public:
		SupplierAdmin (EventChannel& channel) :
			channel_ (channel)
		{}

		CosEventChannelAdmin::ProxyPushConsumer::_ref_type obtain_push_consumer ()
		{
			return nullptr;
		}

		CosEventChannelAdmin::ProxyPullConsumer::_ref_type obtain_pull_consumer ()
		{
			return nullptr;
		}

	private:
		EventChannel& channel_;
	};

	servant_reference <ConsumerAdmin> consumer_admin_;
	servant_reference <SupplierAdmin> supplier_admin_;
	Nirvana::Core::EventSync event_;
};

}
}
