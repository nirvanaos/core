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
#include "EventChannel.h"
#include "ServantProxyObject.h"
#include "../CORBA/CosTypedEventChannelAdmin_s.h"

namespace CORBA {
namespace Core {

class TypedEventChannel :
	public servant_traits <CosTypedEventChannelAdmin::TypedEventChannel>::Servant <TypedEventChannel>,
	public EventChannelBase
{
public:
	CosTypedEventChannelAdmin::TypedConsumerAdmin::_ref_type for_consumers ()
	{
		check_exist ();
		if (!consumer_admin_)
			consumer_admin_ = make_reference <ConsumerAdmin> (std::ref (*this));
		return consumer_admin_->_this ();
	}

	CosTypedEventChannelAdmin::TypedSupplierAdmin::_ref_type for_suppliers ()
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

	CosTypedEventChannelAdmin::TypedProxyPushConsumer::_ref_type obtain_typed_push_consumer (
		const IDL::String& supported_interface)
	{
		if (supported_interface.empty ())
			throw BAD_PARAM ();
		return push_consumers_.create <PushConsumer> (std::ref (*this), std::ref (supported_interface));
	}

	CosTypedEventChannelAdmin::TypedProxyPullSupplier::_ref_type obtain_typed_pull_supplier (
		const IDL::String& supported_interface)
	{
		if (supported_interface.empty ())
			throw BAD_PARAM ();
		return pull_suppliers_.create <PullSupplier> (std::ref (*this), std::ref (supported_interface));
	}

	CosEventChannelAdmin::ProxyPullConsumer::_ref_type obtain_typed_pull_consumer (
		const IDL::String& uses_interface)
	{
		// I don't think that it will really needed.
		throw NO_IMPLEMENT ();
		/*
		if (uses_interface.empty ())
			throw BAD_PARAM ();
		Internal::ProxyFactory::_ref_type pf = get_proxy_factory (uses_interface);
		TypeCode::_ref_type param_type = check_pull (*pf->metadata ());
		if (!param_type || !set_event_type (param_type))
			throw CosTypedEventChannelAdmin::NoSuchImplementation ();
		return EventChannelBase::obtain_pull_consumer ();
		*/
	}

	CosEventChannelAdmin::ProxyPushSupplier::_ref_type obtain_typed_push_supplier (
		IDL::String& uses_interface)
	{
		if (uses_interface.empty ())
			throw BAD_PARAM ();
		Internal::ProxyFactory::_ref_type pf = get_proxy_factory (uses_interface);
		TypeCode::_ref_type param_type = check_push (*pf->metadata ());
		if (!param_type || !set_event_type (param_type))
			throw CosTypedEventChannelAdmin::NoSuchImplementation ();
		return push_suppliers_.create <PushSupplier> (std::ref (*this), std::move (uses_interface));
	}

private:
	static void deactivate_object (Object::_ptr_type obj,
		PortableServer::POA::_ptr_type adapter) noexcept;

	void push (Internal::IORequest::_ptr_type rq)
	{
		Any ev;
		if (event_type_->kind () != TCKind::tk_void)
			Internal::Type <Any>::unmarshal (event_type_, rq, ev);
		rq->unmarshal_end ();
		EventChannelBase::push (ev);
	}

	virtual void push (const Any& data) override;

	static TypeCode::_ptr_type check_push (const Internal::InterfaceMetadata& metadata);
	static TypeCode::_ptr_type check_pull (const Internal::InterfaceMetadata& metadata);

	bool set_event_type (TypeCode::_ref_type& event_type)
	{
		if (!event_type_) {
			event_type_ = std::move (event_type);
			return true;
		} else
			return event_type->equivalent (event_type_);
	}

	static Internal::ProxyFactory::_ref_type get_proxy_factory (const IDL::String& uses_interface);

	class PushConsumer;

	class ProxyBase : public ServantProxyObject
	{
	protected:
		ProxyBase (PortableServer::Servant servant, const IDL::String& interface_id) :
			ServantProxyObject (servant, interface_id)
		{
			// Interface must not have base interfaces
			if (interfaces ().size () != 1)
				throw CosTypedEventChannelAdmin::InterfaceNotSupported ();
		}

		~ProxyBase ()
		{
			delete_servant (true);
		}
	};

	class ProxyPush : public ProxyBase
	{
	public:
		ProxyPush (PushConsumer& servant, const IDL::String& interface_id) :
			ProxyBase (&servant, interface_id)
		{
			// Check interface requirements and obtain event type
			TypeCode::_ref_type param_type = check_push (*interfaces ().front ().interface_metadata);
			if (!param_type || !servant.channel ().set_event_type (param_type))
				throw CosTypedEventChannelAdmin::InterfaceNotSupported ();

			// Hack interface metadata
			interface_metadata_ = interfaces ().front ().metadata ();
			operation_ = interface_metadata_.operations.p [0];
			interface_metadata_.operations.p = &operation_;
			operation_.invoke = &Internal::RqProcWrapper <PushConsumer>::call <rq_push>;
			set_interface_metadata (&interface_metadata_, reinterpret_cast <Interface*> ((void*)&servant));
		}

	private:
		static void rq_push (PushConsumer* servant, Internal::IORequest::_ptr_type call);

	private:
		Internal::Operation operation_;
		Internal::InterfaceMetadata interface_metadata_;
	};

	class PushConsumer : public PushConsumerBase,
		public servant_traits <CosTypedEventChannelAdmin::TypedProxyPushConsumer>::Servant <PushConsumer>
	{
	public:
		PushConsumer (TypedEventChannel& channel, const IDL::String& supported_interface) :
			PushConsumerBase (channel),
			proxy_push_ (new ProxyPush (*this, supported_interface))
		{}

		~PushConsumer ()
		{
			delete proxy_push_;
		}

		void push (Internal::IORequest::_ptr_type call)
		{
			if (channel_)
				channel ().push (call);
		}

		void push (const Any& data)
		{
			PushConsumerBase::push (data);
		}

		Object::_ref_type get_typed_consumer () const noexcept
		{
			return proxy_push_->get_proxy ();
		}

		virtual void destroy (PortableServer::POA::_ptr_type adapter) noexcept override
		{
			deactivate_object (proxy_push_->get_proxy (), adapter);
			PushConsumerBase::destroy (this, adapter);
		}

		TypedEventChannel& channel () const noexcept
		{
			assert (channel_);
			return static_cast <TypedEventChannel&> (*channel_);
		}

	private:
		ProxyPush* proxy_push_;
	};

	class PullSupplier;

	class ProxyPull : public ProxyBase
	{
	public:
		ProxyPull (PullSupplier& servant, const IDL::String& interface_id) :
			ProxyBase (&servant, interface_id)
		{
			// Check interface requirements and obtain event type
			TypeCode::_ref_type param_type = check_pull (*interfaces ().front ().interface_metadata);
			if (!param_type || !servant.channel ().set_event_type (param_type))
				throw CosTypedEventChannelAdmin::InterfaceNotSupported ();

			// Hack interface metadata
			interface_metadata_ = interfaces ().front ().metadata ();
			operations_ [0] = interface_metadata_.operations.p [0];
			operations_ [1] = interface_metadata_.operations.p [1];
			interface_metadata_.operations.p = operations_;
			operations_ [0].invoke = &Internal::RqProcWrapper <PullSupplier>::call <rq_pull>;
			operations_ [1].invoke = &Internal::RqProcWrapper <PullSupplier>::call <rq_try>;
			set_interface_metadata (&interface_metadata_, reinterpret_cast <Interface*> ((void*)&servant));
		}

	private:
		static void rq_pull (PullSupplier* servant, Internal::IORequest::_ptr_type call);
		static void rq_try (PullSupplier* servant, Internal::IORequest::_ptr_type call);

	private:
		Internal::Operation operations_ [2];
		Internal::InterfaceMetadata interface_metadata_;
	};

	class PullSupplier : public PullSupplierBase,
		public servant_traits <CosTypedEventChannelAdmin::TypedProxyPullSupplier>::Servant <PullSupplier>
	{
	public:
		PullSupplier (TypedEventChannel& channel, const IDL::String& supported_interface) :
			PullSupplierBase (channel),
			proxy_pull_ (new ProxyPull (*this, supported_interface))
		{}

		~PullSupplier ()
		{
			delete proxy_pull_;
		}

		Object::_ref_type get_typed_supplier () const noexcept
		{
			return proxy_pull_->get_proxy ();
		}

		virtual void destroy (PortableServer::POA::_ptr_type adapter) noexcept override
		{
			deactivate_object (proxy_pull_->get_proxy (), adapter);
			PullSupplierBase::destroy (this, adapter);
		}

		TypedEventChannel& channel () const noexcept
		{
			assert (channel_);
			return static_cast <TypedEventChannel&> (*channel_);
		}

		Any try_pull (bool& has_event)
		{
			Any ev = PullSupplierBase::try_pull (has_event);
			if (!has_event) {
				TypeCode::_ptr_type et = channel ().event_type_;
				if (et && et->kind () != TCKind::tk_void)
					ev.construct (et);
			}
			return ev;
		}

	private:
		ProxyPull* proxy_pull_;
	};

	class PushSupplier : public PushSupplierBase,
		public servant_traits <CosEventChannelAdmin::ProxyPushSupplier>::Servant <PushSupplier>
	{
	public:
		PushSupplier (EventChannelBase& channel, IDL::String&& uses_interface) :
			PushSupplierBase (channel),
			uses_interface_ (std::move (uses_interface))
		{}

		virtual void destroy (PortableServer::POA::_ptr_type adapter) noexcept override
		{
			proxy_ = nullptr;
			PushSupplierBase::destroy (this, adapter);
		}

		void connect_push_consumer (CosEventComm::PushConsumer::_ptr_type push_consumer)
		{
			CosTypedEventComm::TypedPushConsumer::_ref_type typed =
				CosTypedEventComm::TypedPushConsumer::_narrow (push_consumer);
			if (!typed)
				throw CosEventChannelAdmin::TypeError ();

			Object::_ptr_type obj = typed->get_typed_consumer ();
			ProxyManager* proxy = ProxyManager::cast (obj);
			if (!proxy)
				throw BAD_PARAM ();
			const ProxyManager::InterfaceEntry* ie = proxy->find_interface (uses_interface_);
			if (!ie)
				throw CosEventChannelAdmin::TypeError ();
			TypeCode::_ptr_type tc = check_push (ie->metadata ());
			const TypedEventChannel& channel = static_cast <const TypedEventChannel&> (check_exist ());
			if (!tc || !tc->equivalent (channel.event_type_))
				throw CosEventChannelAdmin::TypeError ();
			PushSupplierBase::connect_push_consumer (push_consumer);
			proxy_ = proxy;
			op_idx_ = Internal::make_op_idx ((UShort)(ie - proxy_->interfaces ().begin () + 1), 0);
		}

		void disconnect_push_supplier () noexcept
		{
			proxy_ = nullptr;
			PushSupplierBase::disconnect_push_supplier ();
		}

		virtual void push (const Any& data) noexcept override
		{
			assert (proxy_);
			try {
				Internal::IORequest::_ptr_type rq = proxy_->create_request (op_idx_, 0, nullptr);
				data.type ()->n_marshal_in (data.data (), 1, rq);
				rq->invoke ();
			} catch (...) {}
		}

	private:
		IDL::String uses_interface_;
		servant_reference <ProxyManager> proxy_;
		Internal::OperationIndex op_idx_;
	};

	class SupplierAdmin : public SupplierAdminBase,
		public servant_traits <CosTypedEventChannelAdmin::TypedSupplierAdmin>::Servant <SupplierAdmin>
	{
	public:
		SupplierAdmin (EventChannelBase& channel) :
			SupplierAdminBase (channel)
		{}

		CosTypedEventChannelAdmin::TypedProxyPushConsumer::_ref_type obtain_typed_push_consumer (
			const IDL::String& supported_interface)
		{
			return static_cast <TypedEventChannel&> (check_exist ())
				.obtain_typed_push_consumer (supported_interface);
		}

		CosEventChannelAdmin::ProxyPullConsumer::_ref_type obtain_typed_pull_consumer (
			const IDL::String& uses_interface)
		{
			return static_cast <TypedEventChannel&> (check_exist ())
				.obtain_typed_pull_consumer (uses_interface);
		}

		virtual void destroy (PortableServer::POA::_ptr_type adapter) noexcept override
		{
			ChildObject::destroy (this, adapter);
		}
	};

	class ConsumerAdmin : public ConsumerAdminBase,
		public servant_traits <CosTypedEventChannelAdmin::TypedConsumerAdmin>::Servant <ConsumerAdmin>
	{
	public:
		ConsumerAdmin (EventChannelBase& channel) :
			ConsumerAdminBase (channel)
		{}

		CosTypedEventChannelAdmin::TypedProxyPullSupplier::_ref_type obtain_typed_pull_supplier (
			const IDL::String& supported_interface)
		{
			return static_cast <TypedEventChannel&> (check_exist ())
				.obtain_typed_pull_supplier (supported_interface);
		}

		CosEventChannelAdmin::ProxyPushSupplier::_ref_type obtain_typed_push_supplier (
			IDL::String& uses_interface)
		{
			return static_cast <TypedEventChannel&> (check_exist ())
				.obtain_typed_push_supplier (uses_interface);
		}

		virtual void destroy (PortableServer::POA::_ptr_type adapter) noexcept override
		{
			ChildObject::destroy (this, adapter);
		}
	};

private:
	servant_reference <ConsumerAdmin> consumer_admin_;
	servant_reference <SupplierAdmin> supplier_admin_;
	TypeCode::_ref_type event_type_;
};

}
}
