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
#include "TypedEventChannel.h"
#include "../Binder.h"

using namespace CosTypedEventChannelAdmin;

namespace CORBA {
using namespace Internal;
namespace Core {

void TypedEventChannel::ProxyPush::rq_push (PushConsumer* servant, IORequest::_ptr_type call)
{
	servant->push (call);
}

void TypedEventChannel::ProxyPull::rq_pull (PullSupplier* servant, Internal::IORequest::_ptr_type call)
{
	Any ev = servant->pull ();
	ev.type ()->n_marshal_out (ev.data (), 1, call);
}

void TypedEventChannel::ProxyPull::rq_try (PullSupplier* servant, Internal::IORequest::_ptr_type call)
{
	bool has_event;
	Any ev = servant->try_pull (has_event);
	Type <bool>::marshal_out (has_event, call);
	Type <Any>::marshal_out (ev, call);
}

TypeCode::_ref_type TypedEventChannel::check_push (const Internal::InterfaceMetadata& metadata)
{
	// Interface must not have base interfaces
	if (metadata.interfaces.size != 1)
		return nullptr;

	// Interface must have only one operation
	const Internal::CountedArray <Internal::Operation>& ops = metadata.operations;
	if (ops.size != 1)
		return nullptr;
	// Interface operation must have only one input parameter
	const Internal::Operation& op = ops.p [0];
	if (op.input.size != 1 || op.output.size != 0 || op.return_type)
		return nullptr;

	return (op.input.p [0].type) ();
}

TypeCode::_ref_type TypedEventChannel::check_pull (const Internal::InterfaceMetadata& metadata)
{
	// Interface must not have base interfaces
	if (metadata.interfaces.size != 1)
		return nullptr;

	// Interface must have exactly 2 operations
	const Internal::CountedArray <Internal::Operation>& ops = metadata.operations;
	if (ops.size != 2)
		return nullptr;

	// Pull operation must have only one output parameter
	const Internal::Operation& op_pull = ops.p [0];
	if (op_pull.output.size != 1 || op_pull.input.size != 0 || op_pull.return_type)
		return nullptr;

	TypeCode::_ref_type param_type = (op_pull.output.p [0].type) ();

	// Try operation must have bool return type
	const Internal::Operation& op_try = ops.p [1];
	if (op_try.output.size != 1 || op_try.input.size != 0 || !op_try.return_type
		|| !(op_try.return_type) ()->equivalent (_tc_boolean) || !(op_try.output.p [0].type) ()->equivalent (param_type))
		return nullptr;

	return param_type;
}

void TypedEventChannel::deactivate_object (Object::_ptr_type obj,
	PortableServer::POA::_ptr_type adapter) noexcept
{
	try {
		adapter->deactivate_object (adapter->reference_to_id (obj));
	} catch (...) {
	}
}

ProxyFactory::_ref_type TypedEventChannel::get_proxy_factory (const IDL::String& uses_interface)
{
	try {
		return Nirvana::Core::Binder::bind_interface <ProxyFactory> (uses_interface);
	} catch (...) {
		throw NoSuchImplementation ();
	}
}

}
}
