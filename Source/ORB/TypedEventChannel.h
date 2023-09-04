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

template <class S, class Base, class I>
class AddInterface :
	public Base,
	public Internal::InterfaceImpl <S, I>,
	public Internal::ServantTraits <S>,
	public Internal::LifeCycleRefCnt <S>
{
	typedef Internal::ServantTraits <S> ServantTraits;
	typedef Internal::LifeCycleRefCnt <S> LifeCycle;

public:
	typedef I PrimaryInterface;

	using ServantTraits::_implementation;
	using ServantTraits::_wide_object;
	using ServantTraits::_wide;
	using LifeCycle::__duplicate;
	using LifeCycle::__release;
	using LifeCycle::_duplicate;
	using LifeCycle::_release;

	Internal::Interface* _query_interface (Internal::String_in id) noexcept
	{
		if (id.empty () || Internal::RepId::compatible (Internal::RepIdOf <I>::id, id))
			return static_cast <Internal::Bridge <I>*> (this);
		else
			return Base::_query_interface (id);
	}

	Internal::I_ref <I> _this ()
	{
		return Base::_get_proxy ().template downcast <I> ();
	}

protected:
	template <class ... Args>
	AddInterface (Args ... args) :
		Base (std::forward <Args> (args)...)
	{}
};

namespace Core {

class TypedEventChannel :
	public servant_traits <CosTypedEventChannelAdmin::TypedEventChannel>::Servant <TypedEventChannel>,
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

	CosTypedEventChannelAdmin::TypedProxyPushConsumer::_ref_type obtain_typed_push_consumer (
		const IDL::String& supported_interface)
	{
		return push_consumers_.create <PushConsumer> (std::ref (*this), std::ref (supported_interface));
	}

	CosTypedEventChannelAdmin::TypedProxyPullSupplier::_ref_type obtain_typed_pull_supplier (
		const IDL::String& supported_interface)
	{
		return pull_suppliers_.create <PullSupplier> (std::ref (*this), std::ref (supported_interface));
	}

	CosEventChannelAdmin::ProxyPullConsumer::_ref_type obtain_typed_pull_consumer (
		const IDL::String& uses_interface)
	{
		Internal::ProxyFactory::_ref_type pf = get_proxy_factory (uses_interface);
		TypeCode::_ref_type param_type = check_pull (*pf->metadata ());
		if (!param_type || !set_event_type (param_type))
			throw CosTypedEventChannelAdmin::NoSuchImplementation ();
		return EventChannelBase::obtain_pull_consumer ();
	}

	CosEventChannelAdmin::ProxyPushSupplier::_ref_type obtain_typed_push_supplier (
		const IDL::String& uses_interface)
	{
		Internal::ProxyFactory::_ref_type pf = get_proxy_factory (uses_interface);
		TypeCode::_ref_type param_type = check_push (*pf->metadata ());
		if (!param_type || !set_event_type (param_type))
			throw CosTypedEventChannelAdmin::NoSuchImplementation ();
		return EventChannelBase::obtain_push_supplier ();
	}

private:
	static void deactivate_object (Object::_ptr_type obj,
		PortableServer::POA::_ptr_type adapter) noexcept;

	void push (Internal::IORequest::_ptr_type rq)
	{
		Any ev;
		Internal::Type <Any>::unmarshal (event_type_, rq, ev);
		rq->unmarshal_end ();
		EventChannelBase::push (ev);
	}

	virtual void push (const Any& data) override
	{
		if (!event_type_ || data.type ()->equivalent (event_type_))
			EventChannelBase::push (data);
	}

	static TypeCode::_ref_type check_push (const Internal::InterfaceMetadata& metadata);
	static TypeCode::_ref_type check_pull (const Internal::InterfaceMetadata& metadata);

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

	class ProxyPush : public ServantProxyObject
	{
	public:
		ProxyPush (PushConsumer& servant, const IDL::String& interface_id) :
			ServantProxyObject (&servant, interface_id)
		{
			// Interface must not have base interfaces
			if (interfaces ().size () != 1)
				throw CosTypedEventChannelAdmin::InterfaceNotSupported ();

			// Check interface requirements and obtain event type
			TypeCode::_ref_type param_type = check_push (*interfaces ().front ().interface_metadata);
			if (!param_type || !servant.channel ().set_event_type (param_type))
				throw CosTypedEventChannelAdmin::InterfaceNotSupported ();

			// Hack interface metadata
			interface_metadata_ = interfaces ().front ().metadata ();
			operation_ = interface_metadata_.operations.p [0];
			interface_metadata_.operations.p = &operation_;
			operation_.invoke = &Internal::RqProcWrapper <PushConsumer>::call <rq_push>;
			set_interface_metadata (&interface_metadata_, reinterpret_cast <Interface*> (&servant));
		}

	private:
		static void rq_push (PushConsumer* servant, Internal::IORequest::_ptr_type call);

	private:
		Internal::Operation operation_;
		Internal::InterfaceMetadata interface_metadata_;
	};

	class PushConsumer : public AddInterface <PushConsumer, EventChannelBase::PushConsumer,
		CosTypedEventChannelAdmin::TypedProxyPushConsumer>
	{
		typedef AddInterface <PushConsumer, EventChannelBase::PushConsumer,
			CosTypedEventChannelAdmin::TypedProxyPushConsumer> Base;

	public:
		PushConsumer (TypedEventChannel& channel, const IDL::String& supported_interface) :
			Base (std::ref (channel)),
			proxy_push_ (*this, supported_interface)
		{}

		void push (Internal::IORequest::_ptr_type call)
		{
			if (channel_)
				channel ().push (call);
		}

		Object::_ref_type get_typed_consumer () const noexcept
		{
			return proxy_push_.get_proxy ();
		}

		virtual void destroy (PortableServer::POA::_ptr_type adapter) noexcept override
		{
			deactivate_object (proxy_push_.get_proxy (), adapter);
			Base::destroy (adapter);
		}

		TypedEventChannel& channel () const noexcept
		{
			assert (channel_);
			return static_cast <TypedEventChannel&> (*channel_);
		}

	private:
		ProxyPush proxy_push_;
	};

	class PullSupplier;

	class ProxyPull : public ServantProxyObject
	{
	public:
		ProxyPull (PullSupplier& servant, const IDL::String& interface_id) :
			ServantProxyObject (&servant, interface_id)
		{
			// Interface must not have base interfaces
			if (interfaces ().size () != 1)
				throw CosTypedEventChannelAdmin::InterfaceNotSupported ();

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
			set_interface_metadata (&interface_metadata_, reinterpret_cast <Interface*> (&servant));
		}

	private:
		static void rq_pull (PullSupplier* servant, Internal::IORequest::_ptr_type call);
		static void rq_try (PullSupplier* servant, Internal::IORequest::_ptr_type call);

	private:
		Internal::Operation operations_ [2];
		Internal::InterfaceMetadata interface_metadata_;
	};

	class PullSupplier : public AddInterface <PullSupplier, EventChannelBase::PullSupplier,
		CosTypedEventChannelAdmin::TypedProxyPullSupplier>
	{
		typedef AddInterface <PullSupplier, EventChannelBase::PullSupplier,
			CosTypedEventChannelAdmin::TypedProxyPullSupplier> Base;

	public:
		PullSupplier (TypedEventChannel& channel, const IDL::String& supported_interface) :
			Base (std::ref (channel)),
			proxy_pull_ (*this, supported_interface)
		{}

		Object::_ref_type get_typed_supplier () const noexcept
		{
			return proxy_pull_.get_proxy ();
		}

		virtual void destroy (PortableServer::POA::_ptr_type adapter) noexcept override
		{
			deactivate_object (proxy_pull_.get_proxy (), adapter);
			Base::destroy (adapter);
		}

		TypedEventChannel& channel () const noexcept
		{
			assert (channel_);
			return static_cast <TypedEventChannel&> (*channel_);
		}

	private:
		ProxyPull proxy_pull_;
	};

	class SupplierAdmin : public AddInterface <SupplierAdmin, EventChannelBase::SupplierAdmin,
		CosTypedEventChannelAdmin::TypedSupplierAdmin>
	{
		typedef AddInterface <SupplierAdmin, EventChannelBase::SupplierAdmin,
			CosTypedEventChannelAdmin::TypedSupplierAdmin> Base;

	public:
		SupplierAdmin (EventChannelBase& channel) :
			Base (std::ref (channel))
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
	};

	class ConsumerAdmin : public AddInterface <ConsumerAdmin, EventChannelBase::ConsumerAdmin,
		CosTypedEventChannelAdmin::TypedConsumerAdmin>
	{
		typedef AddInterface <ConsumerAdmin, EventChannelBase::ConsumerAdmin,
			CosTypedEventChannelAdmin::TypedConsumerAdmin> Base;

	public:
		ConsumerAdmin (EventChannelBase& channel) :
			Base (std::ref (channel))
		{}

		CosTypedEventChannelAdmin::TypedProxyPullSupplier::_ref_type obtain_typed_pull_supplier (
			const IDL::String& supported_interface)
		{
			return static_cast <TypedEventChannel&> (check_exist ())
				.obtain_typed_pull_supplier (supported_interface);
		}

		CosEventChannelAdmin::ProxyPushSupplier::_ref_type obtain_typed_push_supplier (
			const IDL::String& uses_interface)
		{
			return static_cast <TypedEventChannel&> (check_exist ())
				.obtain_typed_push_supplier (uses_interface);
		}
	};

private:
	servant_reference <ConsumerAdmin> consumer_admin_;
	servant_reference <SupplierAdmin> supplier_admin_;
	TypeCode::_ref_type event_type_;
};

}
}
