/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
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
#include <core.h>
#include <CORBA/POA.h>
#include <CORBA/Proxy/TypeCodeException.h>
#include "tc_impex.h"

namespace CORBA {
namespace Nirvana {

template <>
class TypeCodeException <PortableServer::POA::ServantAlreadyActive> :
	public TypeCodeExceptionImpl <PortableServer::POA::ServantAlreadyActive>
{};

template <>
class TypeCodeException <PortableServer::POA::ObjectNotActive> :
	public TypeCodeExceptionImpl <PortableServer::POA::ObjectNotActive>
{};

namespace Core {

StaticI_ptr <PortableServer::POA> g_root_POA = { 0 };

}

}
}

namespace PortableServer {

typedef CORBA::Nirvana::TypeCodeException <POA::ServantAlreadyActive> TC_POA_ServantAlreadyActive;
typedef CORBA::Nirvana::TypeCodeException <POA::ObjectNotActive> TC_POA_ObjectNotActive;

}

INTERFACE_EXC_IMPEX (PortableServer, POA, ServantAlreadyActive)
INTERFACE_EXC_IMPEX (PortableServer, POA, ObjectNotActive)
