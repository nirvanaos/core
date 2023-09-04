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

using namespace CosTypedEventChannelAdmin;

namespace CORBA {
using namespace Internal;
namespace Core {

void TypedEventChannel::ProxyPush::request_proc (PushConsumer* servant, IORequest::_ptr_type call)
{

}

void TypedEventChannel::deactivate_object (Object::_ptr_type obj,
	PortableServer::POA::_ptr_type adapter) noexcept
{
	try {
		adapter->deactivate_object (adapter->reference_to_id (obj));
	} catch (...) {
	}
}

}
}
