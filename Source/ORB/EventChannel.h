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
#include "../MapUnorderedUnstable.h"
#include "../UserAllocator.h"
#include <queue>

namespace CORBA {
namespace Core {

typedef Nirvana::Core::ImplDynamicSync <Any> SharedAny;

class EventChannel :
	public servant_traits <CosEventChannelAdmin::EventChannel>::Servant <EventChannel>
{
public:
	EventChannel () :
		destroyed_ (false)
	{}

	CosEventChannelAdmin::ConsumerAdmin::_ref_type for_consumers ()
	{
		check_exist ();
		if (!consumer_admin_)
			consumer_admin_ = make_reference <ConsumerAdmin> (std::ref (*this));
		return consumer_admin_->_this ();
	}

	CosEventChannelAdmin::SupplierAdmin::_ref_type for_suppliers ()
	{
		check_exist ();
		if (!supplier_admin_)
			supplier_admin_ = make_reference <SupplierAdmin> (std::ref (*this));
		return supplier_admin_->_this ();
	}

	void destroy ()
	{
		check_exist ();
		destroyed_ = true;

		PortableServer::POA::_ref_type adapter = _default_POA ();
		try {
			adapter->deactivate_object (adapter->servant_to_id (this));
		} catch (...) {}

		push_suppliers_.destroy (adapter);
		push_consumers_.destroy (adapter);
		pull_suppliers_.destroy (adapter);
		pull_consumers_.destroy (adapter);

		if (consumer_admin_)
			destroy_child (consumer_admin_, adapter);
		if (supplier_admin_)
			destroy_child (supplier_admin_, adapter);
	}

	CosEventChannelAdmin::ProxyPushSupplier::_ref_type obtain_push_supplier ()
	{
		return push_suppliers_.create (*this);
	}

	CosEventChannelAdmin::ProxyPullSupplier::_ref_type obtain_pull_supplier ()
	{
		return pull_suppliers_.create (*this);
	}

	CosEventChannelAdmin::ProxyPushConsumer::_ref_type obtain_push_consumer ()
	{
		return push_consumers_.create (*this);
	}

	CosEventChannelAdmin::ProxyPullConsumer::_ref_type obtain_pull_consumer ()
	{
		return pull_consumers_.create (*this);
	}

private:
	void check_exist () const
	{
		if (destroyed_)
			throw OBJECT_NOT_EXIST ();
	}

	template <class S>
	static void destroy_child (servant_reference <S>& ref, PortableServer::POA::_ptr_type adapter)
	{
		servant_reference <S> tmp = std::move (ref);
		tmp->destroy ();
		try {
			adapter->deactivate_object (adapter->servant_to_id (tmp));
		} catch (...) {
		}
	}

	void push (const Any& data)
	{
		if (pull_suppliers_.empty ()) {
			servant_reference <SharedAny> sany (make_reference <SharedAny> (std::ref (data)));
			for (const auto p : pull_suppliers_) {
				ProxyPullSupplier& sup = static_cast <ProxyPullSupplier&> (*p);
				if (sup.connected ())
					sup.push (*sany);
			}
			event_.signal ();
			event_.reset ();
		}
	}

	class ChildObject
	{
	public:
		void destroy () noexcept
		{
			channel_ = nullptr;
		}

	protected:
		ChildObject (EventChannel& channel) :
			channel_ (&channel)
		{}

		EventChannel& check_exist () const
		{
			if (!channel_ || channel_->destroyed_)
				throw OBJECT_NOT_EXIST ();
			return *channel_;
		}

	protected:
		EventChannel* channel_;
	};

	class Children :
		public Nirvana::Core::SetUnorderedUnstable <ChildObject*, std::hash <void*>,
		std::equal_to <void*>, Nirvana::Core::UserAllocator>
	{
	public:
		Children () :
			connected_cnt_ (0)
		{}

		void on_connect () noexcept
		{
			++connected_cnt_;
			assert (connected_cnt_ <= size ());
		}

		void on_disconnect () noexcept
		{
			assert (connected_cnt_);
			--connected_cnt_;
		}

		size_t connected () const noexcept
		{
			return connected_cnt_;
		}

		void on_destruct (ChildObject& child) noexcept
		{
			verify (erase (&child));
			assert (connected_cnt_ <= size ());
		}

	private:
		size_t connected_cnt_;
	};

	template <class S>
	class ChildrenT : public Children
	{
	public:
		auto create (EventChannel& channel)
		{
			servant_reference <S> ref (make_reference <S> (std::ref (channel)));
			insert (ref);
			return ref->_this ();
		}

		void destroy (PortableServer::POA::_ptr_type adapter)
		{
			for (auto p : *this) {
				servant_reference <S> ref (&static_cast <S&> (*p));
				destroy_child (ref, adapter);
			}
		}
	};

	class ProxyPushConsumer :
		public servant_traits <CosEventChannelAdmin::ProxyPushConsumer>::Servant <ProxyPushConsumer>,
		public ChildObject
	{
	public:
		ProxyPushConsumer (EventChannel& channel) :
			ChildObject (channel),
			connected_ (false)
		{}

		~ProxyPushConsumer ()
		{
			disconnect_push_consumer ();
			if (channel_)
				channel_->push_consumers_.on_destruct (*this);
		}

		void connect_push_supplier (CosEventComm::PushSupplier::_ptr_type push_supplier)
		{
			if (connected_)
				throw CosEventChannelAdmin::AlreadyConnected ();
			check_exist ().push_consumers_.on_connect ();
			supplier_ = push_supplier;
			connected_ = true;
		}

		void disconnect_push_consumer () noexcept;

		void push (const Any& data)
		{
			if (!connected_)
				throw CosEventComm::Disconnected ();
			check_exist ().push (data);
		}

		bool connected () const noexcept
		{
			return connected_;
		}

		void destroy () noexcept
		{
			disconnect_push_consumer ();
			ChildObject::destroy ();
		}

	private:
		CosEventComm::PushSupplier::_ref_type supplier_;
		bool connected_;
	};

	class ProxyPullSupplier :
		public servant_traits <CosEventChannelAdmin::ProxyPullSupplier>::Servant <ProxyPullSupplier>,
		public ChildObject
	{
	public:
		ProxyPullSupplier (EventChannel& channel) :
			ChildObject (channel),
			connected_ (false)
		{}

		~ProxyPullSupplier ()
		{
			disconnect_pull_supplier ();
			if (channel_)
				channel_->pull_suppliers_.on_destruct (*this);
		}

		void connect_pull_consumer (CosEventComm::PullConsumer::_ptr_type pull_consumer)
		{
			if (connected_)
				throw CosEventChannelAdmin::AlreadyConnected ();
			check_exist ().pull_suppliers_.on_connect ();
			consumer_ = pull_consumer;
			connected_ = true;
		}

		void disconnect_pull_supplier () noexcept;

		Any pull ()
		{
			if (!connected_)
				throw CosEventComm::Disconnected ();
			if (queue_.empty ())
				check_exist ().event_.wait ();
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

		bool connected () const noexcept
		{
			return connected_;
		}

		void destroy () noexcept
		{
			disconnect_pull_supplier ();
			ChildObject::destroy ();
		}

	private:
		Any pull_internal ()
		{
			servant_reference <SharedAny> ret (std::move (queue_.front ()));
			queue_.pop ();
			return *ret;
		}

	private:
		CosEventComm::PullConsumer::_ref_type consumer_;
		std::queue <servant_reference <SharedAny> > queue_;
		bool connected_;
	};

	class ProxyPullConsumer :
		public servant_traits <CosEventChannelAdmin::ProxyPullConsumer>::Servant <ProxyPullConsumer>,
		public ChildObject
	{
	public:
		ProxyPullConsumer (EventChannel& channel) :
			ChildObject (channel)
		{}

		~ProxyPullConsumer ()
		{
			disconnect_pull_consumer ();
			if (channel_)
				channel_->pull_consumers_.on_destruct (*this);
		}

		void connect_pull_supplier (CosEventComm::PullSupplier::_ptr_type pull_supplier)
		{
			if (!pull_supplier)
				throw BAD_PARAM ();
			if (supplier_)
				throw CosEventChannelAdmin::AlreadyConnected ();
			check_exist ().pull_consumers_.on_connect ();
			supplier_ = pull_supplier;
		}

		void disconnect_pull_consumer () noexcept;

		bool connected () const noexcept
		{
			return bool (supplier_);
		}

		void destroy () noexcept
		{
			disconnect_pull_consumer ();
			ChildObject::destroy ();
		}

	private:
		CosEventComm::PullSupplier::_ref_type supplier_;
	};

	class ProxyPushSupplier :
		public servant_traits <CosEventChannelAdmin::ProxyPushSupplier>::Servant <ProxyPushSupplier>,
		public ChildObject
	{
	public:
		ProxyPushSupplier (EventChannel& channel) :
			ChildObject (channel)
		{}

		~ProxyPushSupplier ()
		{
			disconnect_push_supplier ();
			if (channel_)
				channel_->push_suppliers_.on_destruct (*this);
		}

		void connect_push_consumer (CosEventComm::PushConsumer::_ptr_type push_consumer)
		{
			if (!push_consumer)
				throw BAD_PARAM ();
			if (consumer_)
				throw CosEventChannelAdmin::AlreadyConnected ();
			check_exist ().push_suppliers_.on_connect ();
			consumer_ = push_consumer;
		}

		void disconnect_push_supplier () noexcept;

		bool connected () const noexcept
		{
			return bool (consumer_);
		}

		void destroy () noexcept
		{
			disconnect_push_supplier ();
			ChildObject::destroy ();
		}

	private:
		CosEventComm::PushConsumer::_ref_type consumer_;
	};

	class ConsumerAdmin :
		public servant_traits <CosEventChannelAdmin::ConsumerAdmin>::Servant <ConsumerAdmin>,
		public ChildObject
	{
	public:
		ConsumerAdmin (EventChannel& channel) :
			ChildObject (channel)
		{}

		CosEventChannelAdmin::ProxyPushSupplier::_ref_type obtain_push_supplier ()
		{
			return check_exist ().obtain_push_supplier ();
		}

		CosEventChannelAdmin::ProxyPullSupplier::_ref_type obtain_pull_supplier ()
		{
			return check_exist ().obtain_pull_supplier ();
		}
	};

	class SupplierAdmin :
		public servant_traits <CosEventChannelAdmin::SupplierAdmin>::Servant <SupplierAdmin>,
		public ChildObject
	{
	public:
		SupplierAdmin (EventChannel& channel) :
			ChildObject (channel)
		{}

		CosEventChannelAdmin::ProxyPushConsumer::_ref_type obtain_push_consumer ()
		{
			return check_exist ().obtain_push_consumer ();
		}

		CosEventChannelAdmin::ProxyPullConsumer::_ref_type obtain_pull_consumer ()
		{
			return check_exist ().obtain_pull_consumer ();
		}
	};

private:
	servant_reference <ConsumerAdmin> consumer_admin_;
	servant_reference <SupplierAdmin> supplier_admin_;
	Nirvana::Core::EventSync event_;
	ChildrenT <ProxyPushSupplier> push_suppliers_;
	ChildrenT <ProxyPushConsumer> push_consumers_;
	ChildrenT <ProxyPullSupplier> pull_suppliers_;
	ChildrenT <ProxyPullConsumer> pull_consumers_;
	bool destroyed_;
};

}
}
