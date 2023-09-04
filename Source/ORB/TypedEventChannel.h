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

private:
	static void deactivate_object (Object::_ptr_type obj,
		PortableServer::POA::_ptr_type adapter) noexcept;

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
			// Interface must have only one operation
			const Internal::CountedArray <Internal::Operation>& ops = interfaces ().front ().operations ();
			if (ops.size != 1)
				throw CosTypedEventChannelAdmin::InterfaceNotSupported ();
			// Interface operation must have only one input parameter
			const Internal::Operation& op = ops.p [0];
			if (op.input.size != 1 || op.output.size != 0 || op.return_type)
				throw CosTypedEventChannelAdmin::InterfaceNotSupported ();

			TypeCode::_ref_type param_type = (op.input.p [0].type) ();
			TypeCode::_ref_type& event_type = servant.event_type ();
			if (!event_type)
				event_type = std::move (param_type);
			else if (!param_type->equivalent (event_type))
				throw CosTypedEventChannelAdmin::InterfaceNotSupported ();

			interface_metadata_ = interfaces ().front ().metadata ();
			operation_ = interface_metadata_.operations.p [0];
			interface_metadata_.operations.p = &operation_;
			operation_.invoke = request_proc;
			set_interface_metadata (&interface_metadata_, reinterpret_cast <Interface*> (&servant));
		}

	private:
		static void request_proc (PushConsumer* servant, Internal::IORequest::_ptr_type call);

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

		TypeCode::_ref_type& event_type () const noexcept
		{
			assert (channel_);
			return channel ()->event_type_;
		}

		void push (Internal::IORequest::_ptr_type call);

		Object::_ref_type get_typed_consumer () const noexcept
		{
			return proxy_push_.get_proxy ();
		}

		virtual void destroy (PortableServer::POA::_ptr_type adapter) noexcept override
		{
			deactivate_object (proxy_push_.get_proxy (), adapter);
			Base::destroy (adapter);
		}

	private:
		TypedEventChannel* channel () const noexcept
		{
			return static_cast <TypedEventChannel*> (channel_);
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
			// Interface must have exactly 2 operations
			const Internal::CountedArray <Internal::Operation>& ops = interfaces ().front ().operations ();
			if (ops.size != 2)
				throw CosTypedEventChannelAdmin::InterfaceNotSupported ();


			// Interface operation must have only one input parameter
			const Internal::Operation& op = ops.p [0];
			if (op.input.size != 1 || op.output.size != 0 || op.return_type)
				throw CosTypedEventChannelAdmin::InterfaceNotSupported ();

			TypeCode::_ref_type param_type = (op.input.p [0].type) ();
			TypeCode::_ref_type& event_type = servant.event_type ();
			if (!event_type)
				event_type = std::move (param_type);
			else if (!param_type->equivalent (event_type))
				throw CosTypedEventChannelAdmin::InterfaceNotSupported ();

			interface_metadata_ = interfaces ().front ().metadata ();
			operation_ = interface_metadata_.operations.p [0];
			interface_metadata_.operations.p = &operation_;
			operation_.invoke = request_proc;
			set_interface_metadata (&interface_metadata_, reinterpret_cast <Interface*> (&servant));
		}

	private:
		static bool request_proc (Interface* servant, Interface* call);

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

		TypeCode::_ref_type& event_type () const noexcept
		{
			assert (channel_);
			return channel ()->event_type_;
		}

		Object::_ref_type get_typed_supplier () const noexcept
		{
			return proxy_pull_.get_proxy ();
		}

		virtual void destroy (PortableServer::POA::_ptr_type adapter) noexcept override
		{
			deactivate_object (proxy_pull_.get_proxy (), adapter);
			Base::destroy (adapter);
		}

	private:
		TypedEventChannel* channel () const noexcept
		{
			return static_cast <TypedEventChannel*> (channel_);
		}

	private:
		ProxyPush proxy_pull_;
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
			return static_cast <TypedEventChannel&> (check_exist ()).obtain_typed_push_consumer (supported_interface);
		}

		CosEventChannelAdmin::ProxyPullConsumer::_ref_type obtain_typed_pull_consumer (
			const IDL::String& uses_interface);
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
			const IDL::String& supported_interface);
		CosEventChannelAdmin::ProxyPushSupplier::_ref_type obtain_typed_push_supplier (
			const IDL::String& uses_interface);
	};

	void push (Internal::IORequest::_ptr_type rq)
	{
		Any ev;
		Internal::Type <Any>::unmarshal (event_type_, rq, ev);
		rq->unmarshal_end ();
		EventChannelBase::push (ev);
	}

private:
	servant_reference <ConsumerAdmin> consumer_admin_;
	servant_reference <SupplierAdmin> supplier_admin_;
	TypeCode::_ref_type event_type_;
};

}
}
