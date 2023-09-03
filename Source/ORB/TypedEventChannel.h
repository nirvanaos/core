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
#include "../CORBA/CosTypedEventChannelAdmin_s.h"

namespace CORBA {
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

private:
	class EventProxy;
	class ProxyPush;
	class ProxyPull;

	class SupplierAdmin :
		public servant_traits <CosTypedEventChannelAdmin::TypedSupplierAdmin>::Servant <SupplierAdmin>,
		public EventChannelBase::SupplierAdmin
	{
	public:
		SupplierAdmin (EventChannelBase& channel) :
			EventChannelBase::SupplierAdmin (channel)
		{}

		virtual void destroy (PortableServer::POA::_ptr_type adapter) noexcept override
		{
			ChildObject::destroy (this, adapter);
		}

		CosTypedEventChannelAdmin::TypedProxyPushConsumer::_ref_type obtain_typed_push_consumer (
			const IDL::String& supported_interface);
		CosEventChannelAdmin::ProxyPullConsumer::_ref_type obtain_typed_pull_consumer (
			const IDL::String& uses_interface);
	};

	class ConsumerAdmin :
		public servant_traits <CosTypedEventChannelAdmin::TypedConsumerAdmin>::Servant <ConsumerAdmin>,
		public EventChannelBase::ConsumerAdmin
	{
	public:
		ConsumerAdmin (EventChannelBase& channel) :
			EventChannelBase::ConsumerAdmin (channel)
		{}

		virtual void destroy (PortableServer::POA::_ptr_type adapter) noexcept override
		{
			ChildObject::destroy (this, adapter);
		}

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
