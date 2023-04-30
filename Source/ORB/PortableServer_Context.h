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
#ifndef NIRVANA_ORB_CORE_PORTABLESERVER_CONTEXT_H_
#define NIRVANA_ORB_CORE_PORTABLESERVER_CONTEXT_H_
#pragma once

#include <CORBA/Server.h>
#include "ProxyObject.h"

namespace PortableServer {
namespace Core {

struct Context
{
	POA::_ref_type adapter;
	const ObjectId& object_id;
	CORBA::Object::_ptr_type reference;
	CORBA::Object::_ptr_type servant;

	Context (POA::_ref_type&& poa, const ObjectId& oid, CORBA::Object::_ptr_type ref, CORBA::Core::ServantProxyObject& proxy) :
		adapter (std::move (poa)),
		object_id (oid),
		reference (ref),
		servant (proxy.get_proxy ())
	{}
};

}
}

#endif
