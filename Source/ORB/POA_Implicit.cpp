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
#include "POA_Implicit.h"
#include "POAManagerFactory.h"

using namespace CORBA;
using namespace CORBA::Core;
using namespace Nirvana::Core;

namespace PortableServer {
namespace Core {

ObjectId POA_Implicit::servant_to_id_default (ProxyObject& proxy, bool not_found)
{
	assert (not_found);
	ObjectId oid = generate_object_id ();
	activate_object (ObjectKey (*this, oid), proxy, true);
	return oid;
}

Object::_ref_type POA_Implicit::servant_to_reference_default (ProxyObject& proxy, bool not_found)
{
	return activate_object (ObjectKey (*this), proxy, true)->get_proxy ();
}

}
}