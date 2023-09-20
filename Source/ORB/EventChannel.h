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
#include <CORBA/NoDefaultPOA.h>
#include <CORBA/CosEventChannelAdmin_s.h>
#include "../CoreInterface.h"
#include "../EventSync.h"
#include "../MapUnorderedUnstable.h"
#include "../UserAllocator.h"
#include <queue>

#if defined (__GNUG__) || defined (__clang__)
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif

namespace CORBA {
namespace Core {

typedef Nirvana::Core::ImplDynamicSync <Any> SharedAny;

class EventChannelBase
{
public:
	EventChannelBase () :
		destroyed_ (false)
	{}

	void destroy (PortableServer::ServantBase::_ptr_type servant,
		PortableServer::POA::_ptr_type adapter)
	{
		check_exist ();
		destroyed_ = true;

		deactivate_servant (servant, adapter);

		push_suppliers_.destroy (adapter);
		push_consumers_.destroy (adapter);
		pull_suppliers_.destroy (adapter);
		pull_consumers_.destroy (adapter);
	}

	CosEventChannelAdmin::ProxyPushSupplier::_ref_type obtain_push_supplier ()
	{
		return push_suppliers_.create <PushSupplier> (std::ref (*this));
	}

	CosEventChannelAdmin::ProxyPullSupplier::_ref_type obtain_pull_supplier ()
	{
		return pull_suppliers_.create <PullSupplier> (std::ref (*this));
	}

	CosEventChannelAdmin::ProxyPushConsumer::_ref_type obtain_push_consumer ()
	{
		return push_consumers_.create <PushConsumer> (std::ref (*this));
	}

	CosEventChannelAdmin::ProxyPullConsumer::_ref_type obtain_pull_consumer ()
	{
		return pull_consumers_.create <PullConsumer> (std::ref (*this));
	}

protected:
	void check_exist () const
	{
		if (destroyed_)
			throw OBJECT_NOT_EXIST ();
	}

	static void deactivate_servant (PortableServer::ServantBase::_ptr_type servant,
		PortableServer::POA::_ptr_type adapter) noexcept;

	template <class S>
	static void destroy_child (servant_reference <S> child,
		PortableServer::POA::_ptr_type adapter) noexcept
	{
		if (child) {
			servant_reference <S> tmp (std::move (child));
			tmp->destroy (adapter);
		}
	}

	virtual void push (const Any& data);

	void on_push_supplier_connect ()
	{
		push_suppliers_.on_connect ();
	}

	void on_push_supplier_disconnect ()
	{
		push_suppliers_.on_disconnect ();
	}

	void on_push_consumer_connect ()
	{
		push_consumers_.on_connect ();
	}

	void on_push_consumer_disconnect ()
	{
		push_consumers_.on_disconnect ();
	}

	void on_pull_supplier_connect ()
	{
		pull_suppliers_.on_connect ();
	}

	void on_pull_supplier_disconnect ()
	{
		pull_suppliers_.on_disconnect ();
	}

	void on_pull_consumer_connect ()
	{
		pull_consumers_.on_connect ();
	}

	void on_pull_consumer_disconnect ()
	{
		pull_consumers_.on_disconnect ();
	}

	class ChildObject
	{
	public:
		virtual void destroy (PortableServer::POA::_ptr_type adapter) noexcept = 0;

	protected:
		ChildObject (EventChannelBase& channel) :
			channel_ (&channel)
		{}

		EventChannelBase& check_exist () const
		{
			if (!channel_ || channel_->destroyed_)
				throw OBJECT_NOT_EXIST ();
			return *channel_;
		}

		void destroy (PortableServer::ServantBase::_ptr_type servant,
			PortableServer::POA::_ptr_type adapter) noexcept
		{
			deactivate_servant (servant, adapter);
			channel_ = nullptr;
		}

	protected:
		EventChannelBase* channel_;
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

		template <class S, class ... Args>
		typename S::PrimaryInterface::_ref_type create (Args ... args)
		{
			servant_reference <S> ref (make_reference <S> (std::forward <Args> (args)...));
			insert (&static_cast <ChildObject&> (*ref));
			return ref->_this ();
		}

		void destroy (PortableServer::POA::_ptr_type adapter) noexcept;

	private:
		size_t connected_cnt_;
	};

	class PushConsumerBase : public ChildObject
	{
	public:
		PushConsumerBase (EventChannelBase& channel) :
			ChildObject (channel),
			connected_ (false)
		{}

		~PushConsumerBase ()
		{
			disconnect_push_consumer ();
			if (channel_)
				channel_->push_consumers_.on_destruct (*this);
		}

		void connect_push_supplier (CosEventComm::PushSupplier::_ptr_type push_supplier)
		{
			if (connected_)
				throw CosEventChannelAdmin::AlreadyConnected ();
			check_exist ().on_push_consumer_connect ();
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

	protected:
		void destroy (PortableServer::ServantBase::_ptr_type servant,
			PortableServer::POA::_ptr_type adapter) noexcept
		{
			disconnect_push_consumer ();
			ChildObject::destroy (servant, adapter);
		}

	private:
		CosEventComm::PushSupplier::_ref_type supplier_;
		bool connected_;
	};

	class PushConsumer : public PushConsumerBase,
		public servant_traits <CosEventChannelAdmin::ProxyPushConsumer>::Servant <PushConsumer>
	{
	public:
		PushConsumer (EventChannelBase& channel) :
			PushConsumerBase (channel)
		{}

		virtual void destroy (PortableServer::POA::_ptr_type adapter) noexcept override
		{
			PushConsumerBase::destroy (this, adapter);
		}
	};

	class PullSupplierBase : public ChildObject
	{
	public:
		PullSupplierBase (EventChannelBase& channel) :
			ChildObject (channel),
			connected_ (false)
		{}

		~PullSupplierBase ()
		{
			disconnect_pull_supplier ();
			if (channel_)
				channel_->pull_suppliers_.on_destruct (*this);
		}

		void connect_pull_consumer (CosEventComm::PullConsumer::_ptr_type pull_consumer)
		{
			if (connected_)
				throw CosEventChannelAdmin::AlreadyConnected ();
			check_exist ().on_pull_supplier_connect ();
			consumer_ = pull_consumer;
			connected_ = true;
			event_.reset ();
		}

		void disconnect_pull_supplier () noexcept;

		Any pull ()
		{
			for (;;) {
				if (!connected_)
					throw CosEventComm::Disconnected ();
				if (!queue_.empty ())
					return pull_internal ();
				event_.wait ();
			}
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
			assert (connected_);
			queue_.push (&data);
			event_.signal ();
			event_.reset ();
		}

		bool connected () const noexcept
		{
			return connected_;
		}

	protected:
		void destroy (PortableServer::ServantBase::_ptr_type servant,
			PortableServer::POA::_ptr_type adapter) noexcept
		{
			disconnect_pull_supplier ();
			ChildObject::destroy (servant, adapter);
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
		Nirvana::Core::EventSync event_;
		bool connected_;
	};

	class PullSupplier : public PullSupplierBase,
		public servant_traits <CosEventChannelAdmin::ProxyPullSupplier>::Servant <PullSupplier>
	{
	public:
		PullSupplier (EventChannelBase& channel) :
			PullSupplierBase (channel)
		{}

		virtual void destroy (PortableServer::POA::_ptr_type adapter) noexcept override
		{
			PullSupplierBase::destroy (this, adapter);
		}
	};

	class PullHandler :
		public servant_traits <CosEventComm::AMI_PullSupplierHandler>::Servant <PullHandler>,
		public PortableServer::NoDefaultPOA
	{
	public:
		using PortableServer::NoDefaultPOA::__default_POA;

		PullHandler (EventChannelBase& channel, CosEventComm::PullSupplier::_ptr_type supplier) :
			channel_ (&channel),
			supplier_ (supplier)
		{
			send ();
		}

		void pull (const Any& ami_return_val)
		{
			if (channel_) {
				channel_->push (ami_return_val);
				send ();
			}
		}

		static void pull_excep (Messaging::ExceptionHolder::_ptr_type exh) noexcept
		{}

		static void try_pull (const Any& ami_return_val, bool has_event) noexcept
		{}

		static void try_pull_excep (Messaging::ExceptionHolder::_ptr_type exh) noexcept
		{}

		static void disconnect_pull_supplier () noexcept
		{}

		static void disconnect_pull_supplier_excep (Messaging::ExceptionHolder::_ptr_type exh) noexcept
		{}

		void disconnect () noexcept
		{
			channel_ = nullptr;
			supplier_ = nullptr;
		}

	private:
		void send ()
		{
			supplier_->sendc_pull (_this ());
		}

	private:
		EventChannelBase* channel_;
		CosEventComm::PullSupplier::_ref_type supplier_;
	};

	class PullConsumerBase : public ChildObject
	{
	public:
		PullConsumerBase (EventChannelBase& channel) :
			ChildObject (channel)
		{}

		~PullConsumerBase ()
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
			check_exist ().on_pull_consumer_connect ();
			try {
				handler_ = make_reference <PullHandler> (std::ref (check_exist ()), pull_supplier);
				supplier_ = pull_supplier;
			} catch (...) {
				if (channel_)
					channel_->on_pull_consumer_disconnect ();
				throw;
			}
		}

		void disconnect_pull_consumer () noexcept;

		bool connected () const noexcept
		{
			return bool (supplier_);
		}

	protected:
		void destroy (PortableServer::ServantBase::_ptr_type servant,
			PortableServer::POA::_ptr_type adapter) noexcept
		{
			disconnect_pull_consumer ();
			ChildObject::destroy (servant, adapter);
		}

	private:
		CosEventComm::PullSupplier::_ref_type supplier_;
		servant_reference <PullHandler> handler_;
	};

	class PullConsumer : public PullConsumerBase,
		public servant_traits <CosEventChannelAdmin::ProxyPullConsumer>::Servant <PullConsumer>
	{
	public:
		PullConsumer (EventChannelBase& channel) :
			PullConsumerBase (channel)
		{}

		virtual void destroy (PortableServer::POA::_ptr_type adapter) noexcept override
		{
			PullConsumerBase::destroy (this, adapter);
		}
	};

	class PushSupplierBase : public ChildObject
	{
	public:
		PushSupplierBase (EventChannelBase& channel) :
			ChildObject (channel)
		{}

		~PushSupplierBase ()
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
			check_exist ().on_push_supplier_connect ();
			consumer_ = push_consumer;
		}

		void disconnect_push_supplier () noexcept;

		bool connected () const noexcept
		{
			return bool (consumer_);
		}

		virtual void push (const Any& data) noexcept
		{
			assert (connected ());
			try {
				consumer_->sendc_push (nullptr, data);
			} catch (...) {}
		}

	protected:
		void destroy (PortableServer::ServantBase::_ptr_type servant,
			PortableServer::POA::_ptr_type adapter) noexcept
		{
			disconnect_push_supplier ();
			ChildObject::destroy (servant, adapter);
		}

	private:
		CosEventComm::PushConsumer::_ref_type consumer_;
	};

	class PushSupplier : public PushSupplierBase,
		public servant_traits <CosEventChannelAdmin::ProxyPushSupplier>::Servant <PushSupplier>
	{
	public:
		PushSupplier (EventChannelBase& channel) :
			PushSupplierBase (channel)
		{}

		virtual void destroy (PortableServer::POA::_ptr_type adapter) noexcept override
		{
			PushSupplierBase::destroy (this, adapter);
		}
	};

	class ConsumerAdminBase : public ChildObject
	{
	public:
		ConsumerAdminBase (EventChannelBase& channel) :
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

	class SupplierAdminBase : public ChildObject
	{
	public:
		SupplierAdminBase (EventChannelBase& channel) :
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

protected:
	Children push_suppliers_;
	Children push_consumers_;
	Children pull_suppliers_;
	Children pull_consumers_;
	bool destroyed_;
};

class EventChannel :
	public servant_traits <CosEventChannelAdmin::EventChannel>::Servant <EventChannel>,
	public EventChannelBase
{
public:
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
		PortableServer::POA::_ref_type adapter = _default_POA ();
		EventChannelBase::destroy (this, adapter);
		destroy_child (consumer_admin_, adapter);
		destroy_child (supplier_admin_, adapter);
	}

	class ConsumerAdmin : public ConsumerAdminBase,
		public servant_traits <CosEventChannelAdmin::ConsumerAdmin>::Servant <ConsumerAdmin>
	{
	public:
		ConsumerAdmin (EventChannelBase& channel) :
			ConsumerAdminBase (channel)
		{}

		virtual void destroy (PortableServer::POA::_ptr_type adapter) noexcept override
		{
			ChildObject::destroy (this, adapter);
		}
	};

	class SupplierAdmin : public SupplierAdminBase,
		public servant_traits <CosEventChannelAdmin::SupplierAdmin>::Servant <SupplierAdmin>
	{
	public:
		SupplierAdmin (EventChannelBase& channel) :
			SupplierAdminBase (channel)
		{}

		virtual void destroy (PortableServer::POA::_ptr_type adapter) noexcept override
		{
			ChildObject::destroy (this, adapter);
		}
	};

private:
	servant_reference <ConsumerAdmin> consumer_admin_;
	servant_reference <SupplierAdmin> supplier_admin_;
};

}
}
