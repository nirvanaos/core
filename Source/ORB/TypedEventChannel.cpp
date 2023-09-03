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
#include "ServantProxyObject.h"

using namespace CosTypedEventChannelAdmin;

namespace CORBA {
using namespace Internal;
namespace Core {

class TypedEventChannel::EventProxy :
	public ChildObject,
	public ServantProxyObject
{
protected:
	EventProxy (TypedEventChannel& channel, const IDL::String& interface_id);

	const Operation& operation () const noexcept
	{
		return interfaces ().front ().operations ().p [0];
	}

	void set_request_proc (RequestProc proc) noexcept
	{
		interface_metadata_ = interfaces ().front ().metadata ();
		operation_ = interface_metadata_.operations.p [0];
		interface_metadata_.operations.p = &operation_;
		operation_.invoke = proc;
		set_interface_metadata (&interface_metadata_, reinterpret_cast <Interface*> (channel_));
	}

protected:
	Operation operation_;
	InterfaceMetadata interface_metadata_;
};

TypedEventChannel::EventProxy::EventProxy (TypedEventChannel& channel,
	const IDL::String& interface_id) :
	ChildObject (channel),
	ServantProxyObject (&channel, interface_id)
{
	// Interface must not have base interfaces
	if (interfaces ().size () != 1)
		throw InterfaceNotSupported ();
}

class TypedEventChannel::ProxyPush : EventProxy
{
public:
	ProxyPush (TypedEventChannel& channel, const IDL::String& interface_id) :
		EventProxy (channel, interface_id)
	{
		// Interface must have only one operation
		const Internal::CountedArray <Internal::Operation>& ops = interfaces ().front ().operations ();
		if (ops.size != 1)
			throw InterfaceNotSupported ();
		// Interface operation must have only one input parameter
		const Internal::Operation& op = operation ();
		if (op.input.size != 1 || op.output.size != 0)
			throw InterfaceNotSupported ();

		TypeCode::_ref_type event_type = (op.input.p [0].type) ();
		if (!channel.event_type_)
			channel.event_type_ = std::move (event_type);
		else if (!event_type->equivalent (channel.event_type_))
			throw InterfaceNotSupported ();

		set_request_proc (request_proc);
	}

private:
	static bool request_proc (Interface* servant, Interface* call);
};

bool TypedEventChannel::ProxyPush::request_proc (Interface* servant, Interface* call)
{
	return true;
}

}
}
